#include <opencv2/opencv.hpp>   // Include OpenCV API
#include <fmt/core.h>	      	// formatted terminal output

// Encoding Scheme:
//
// unsigned 16-bit 1-channel value ("depth")
// to / from
// unsigned  8-bit 3-channel values ("red", "green", "blue")
//
//     value    || red    | green  | blue   |
// ------------------------------------------
//            0 || 0      | 0      | 0      | black
// ------------------------------------------
//            1 || 255    | 0      | 0      | red
// ------------------------------------------
//     2 -  255 || 255    | v-1    | 0      | red with green ascending
// ------------------------------------------
//          256 || 255    | 255    | 0      | red + green = yellow
// ------------------------------------------
//   257 -  510 || 511-v  | 255    | 0      | green with red descending
// ------------------------------------------
//          511 || 0      | 255    | 0      | green
// ------------------------------------------
//   512 -  765 || 0      | 255    | v-511  | green with blue ascending
// ------------------------------------------
//          766 || 0      | 255    | 255    | green + blue = cyan
// ------------------------------------------
//   767 - 1020 || 0      | 1021-v | 255    | blue with green descending
// ------------------------------------------
//         1021 || 0      | 0      | 255    | blue
// ------------------------------------------
//  1022 - 1275 || v-1021 | 0      | 255    | blue with red ascending
// ------------------------------------------
//         1276 || 255    | 0      | 255    | blue + red = purple
// ------------------------------------------
//  1277 - 1530 || 255    | 0      | 1531-v | red with blue descending
//

const uint16_t HUE_ENCODER_MAX = 1530;
const float    HUE_MM_SCALE = 0.001f;	// uint16_t depth values in mm
const float    HUE_CM_SCALE = 0.01f;	// uint16_t depth values in cm

float clamp(float value, float lower, float upper)
{
	return std::max(lower, std::min(value, upper));
}

void hue_encode_value(uint16_t v, uint8_t& r, uint8_t& g, uint8_t& b)
{	// Conversion from quantized depth value to RGB color

	if      ( v==0   ) { r=0;      g=0;      b=0;      }
	else if ( v< 256 ) { r=255;    g=v-1;    b=0;      }
	else if ( v< 511 ) { r=511-v;  g=255;    b=0;      }
	else if ( v< 766 ) { r=0;      g=255;    b=v-511;  }
	else if ( v<1021 ) { r=0;      g=1021-v; b=255;    }
	else if ( v<1276 ) { r=v-1021; g=0;      b=255;    }
	else if ( v<1531 ) { r=255;    g=0;      b=1531-v; }
	else               { r=255;    g=0;      b=0;      }
}

cv::Vec3b hue_encode_value(uint16_t v)
{	// Note that this returns a value in OpenCV-compatible BGR format
	cv::Vec3b bgr;
	hue_encode_value(v, bgr[2], bgr[1], bgr[0]);
	return bgr;
}

uint16_t hue_decode_value(uint8_t r, uint8_t g, uint8_t b)
{	// Conversion from RGB color to quantized depth value

	if (b + g + r > 128)
	{
		if (r >= g && r >= b)	// r largest
		{
			if (g >= b) return g - b + 1;    // r largest, then g (   0-255 )
			else        return g - b + 1531; // r largest, then b (1275-1530)
		}
		else if (g >= r && g >= b) return b - r + 511;  // g largest (255 - 765 )
		else if (b >= g && b >= r) return r - g + 1021; // b largest (765 - 1020)
	}
	return 0;
}

uint16_t hue_decode_value(const cv::Vec3b& bgr)
{	// Note that this takes an input value in OpenCV-compatible BGR format
	return hue_decode_value(bgr[2], bgr[1], bgr[0]);
}

class HueCodec
{
	public:

	HueCodec(float depth_min_m, float depth_max_m, float depth_scale=HUE_MM_SCALE, bool inverse_colorization=true)
	: m_depth_min_m(depth_min_m)
	, m_depth_max_m(depth_max_m)
	, m_depth_scale(depth_scale)
	, m_inverse_colorization(inverse_colorization)
	, m_depth_min_u(depth_min_m / depth_scale)
	, m_depth_max_u(depth_max_m / depth_scale)
	{
		if (m_inverse_colorization)
		{
			if (m_depth_min_u == 0.0f) m_depth_min_u = 1E-9;
			m_depth_min_u = 1.0f/m_depth_min_u;
			m_depth_max_u = 1.0f/m_depth_max_u;
		}

		m_depth_range_u = m_depth_max_u - m_depth_min_u;

		// Precompute the encoding lookup table.
		const int enc_count = HUE_ENCODER_MAX + 1;
		m_enc_table.resize(enc_count);
		for (int i=0; i<enc_count; i++)
		{
			cv::Vec3b bgr = hue_encode_value(i);
			m_enc_table[i] = bgr;
		}
	}

	float depth_max_m() const { return m_depth_max_m; }
	float depth_min_m() const { return m_depth_min_m; }
	float depth_scale() const { return m_depth_scale; }

	void encode(const cv::Mat& src, cv::Mat& dst) const
	{
		if (src.empty() || src.type() != CV_16U) return;

		cv::Mat tmp; // Create a temporary matrix to hold the output.
		if (dst.data != src.data) tmp = dst; // Use dst as tmp if it's not the same as src.
		if (tmp.size() != src.size() || tmp.type() != CV_8UC3)
		{	// Initialize tmp if necessary
			tmp = cv::Mat(src.size(), CV_8UC3);
		}

		for (int i=0; i<src.rows; i++)
		{
			for (int j=0; j<src.cols; j++)
			{
				float v = 0;

				float d = src.at<uint16_t>(i, j);
				if (d != 0)
				{
					if (m_inverse_colorization) d = 1.0f / d;
					float scaled = (d - m_depth_min_u) / (m_depth_range_u);
					v = (int)round(HUE_ENCODER_MAX * clamp(scaled, 0.0f, 1.0f));
				}

				tmp.at<cv::Vec3b>(i, j) = m_enc_table[v];
			}
		}

		dst = tmp;
	}

	cv::Mat encode(const cv::Mat& src) const
	{	// Overloaded convenience function to return a Mat
		// Note that this does not allow for pre-allocation or matrix re-use of dst
		cv::Mat dst;
		encode(src, dst);
		return dst;
	}


	void decode(const cv::Mat& src, cv::Mat& dst) const
	{
		if (src.empty() || src.type() != CV_8UC3) return;

		cv::Mat tmp; // Create a temporary matrix to hold the output.
		if (dst.data != src.data) tmp = dst; // Use dst as tmp if it's not the same as src.
		if (tmp.size() != src.size() || tmp.type() != CV_8UC3)
		{	// Initialize tmp if necessary
			tmp = cv::Mat(src.size(), CV_16U);
		}

		for (int i=0; i<src.rows; i++)
		{
			for (int j=0; j<src.cols; j++)
			{
				const cv::Vec3b& bgr(src.at<cv::Vec3b>(i, j));
				uint16_t v = hue_decode_value(bgr);

				float d;
				if (v == 0 || v > HUE_ENCODER_MAX) d = 0;
				else
				{
					d = (m_depth_min_u + (m_depth_range_u * v / HUE_ENCODER_MAX));
					if (m_inverse_colorization) d = 1.0 / d;
				}

				tmp.at<uint16_t>(i, j) = round(d);
			}
		}

		dst = tmp;
	}

	cv::Mat decode(const cv::Mat& src) const
	{	// Overloaded convenience function to return a Mat
		// Note that this does not allow for pre-allocation or matrix re-use of dst
		cv::Mat dst;
		decode(src, dst);
		return dst;
	}

	public:
	float m_depth_min_m, m_depth_max_m, m_depth_scale;
	bool m_inverse_colorization;

	private:
	float m_depth_min_u, m_depth_max_u, m_depth_range_u;
	std::vector<cv::Vec3b> m_enc_table;
};
