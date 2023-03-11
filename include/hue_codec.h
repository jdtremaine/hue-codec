#include <opencv2/opencv.hpp>   // Include OpenCV API

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
{	// Conversion from a 16-bit unsigned integer value in the range of 0 to 1530
	// to an RGB color

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
{	// Conversion from a 16-bit unsigned integer in the range of 0 to 1530
	// and returns an OpenCV Vec3b (OpenCV BGR pixel value)
	// Note that this Vec3b is in OpenCV-compatible BGR format
	cv::Vec3b bgr;
	hue_encode_value(v, bgr[2], bgr[1], bgr[0]);
	return bgr;
}

uint16_t hue_decode_value(uint8_t r, uint8_t g, uint8_t b)
{	// Conversion from RGB color values
	// to a quantized depth value in the range of 0 to 1530

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
{	// Conversion from an OpenCV Vec3b (OpenCV BGR pixel value)
	// to a quantized depth value in the range of 0 to 1530
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
	{	// Convert from an OpenCV Mat of unsigned 16-bit integers
		// to a 3-channel, 8-bit (BGR) hue-encoded OpenCV Mat.
		// This function handles scaling of the input integer values from their
		// existing values into the 0-1530 range required for encoding.
		// Scaling is handled using m_depth_min_m, depth_max_m, and m_depth_scale
		// values provided on object initialization.
		//
		// This function also handles inverse colorization if specified.

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
	{	// Convert from a 3-channel, 8-bit (BGR) hue-encoded OpenCV Mat.
		// to an OpenCV Mat of unsigned 16-bit integers
		//
		// Scaling is handled using m_depth_min_m, depth_max_m, and m_depth_scale
		// values provided on object initialization.
		// So long as these values are the same as were used for hue encoding,
		// this function will scale the values back to their original.
		//
		// This function also handles inverse colorization if specified.

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

uint16_t calc_median(std::vector<uint16_t>& vec)
{	// Efficient calculation of the median value.
	// Note that this calculates the median as if it's an odd-sized vector.
	// A standard median calculation is not used to improve performance.
	// Note that the input array will be modified as nth_element is used.

	if (vec.size() < 3) return 0;

	// Partial sort of the vector to determine the median value
	size_t n = vec.size() / 2;
	std::nth_element(vec.begin(), vec.begin()+n, vec.end());
	return vec[n];
}


bool get_above_diff_threshold(uint16_t val, uint16_t median, float diff_threshold)
{	// Check to see that the maximum value from the vector is higher than
	// the diff_threshold (percentage difference)
	float percent_difference = ((float)val - (float)median) / (float)median;

	return percent_difference > diff_threshold;
}


void median_filter(cv::Mat& src, cv::Mat& dst, int kernel_size=2, float diff_threshold=0.0f)
{
	// Apply a median filter to the depth image
	// This can be done as a postprocessing step to eliminate compression artefacts

	// diff_threshold is a percentage in terms of (maximum value in kernel - median)/median
	// This filter replaces values above the threshold with local median
	// Note that a diff_treshold of 0 will filter all data.

	// kernel_size is given in pixels
	// The local median is the median value within a square surrounding the pixel.
	// The square (aka kernel) has length 2*kernel_size+1

	// If possible this filter will fill in zero areas with the local median value

	if (src.empty() || src.type() != CV_16U) return;

	if (kernel_size == 0)
	{
		dst = src;
		return;
	}

	cv::Mat tmp; // Create a temporary matrix to hold the output.
	if (dst.data != src.data) tmp = dst; // Use dst as tmp if it's not the same as src.
	if (tmp.size() != src.size() || tmp.type() != CV_8UC3)
	{	// Initialize tmp if necessary
		tmp = cv::Mat(src.size(), CV_16U, cv::Scalar(0));
	}

	std::vector<unsigned short> target_kernel;
	for (int i=kernel_size; i<src.rows-kernel_size; i++)
	{
		for (int j=kernel_size; j<src.cols-kernel_size; j++)
		{
			target_kernel.clear();
			target_kernel.reserve(pow(2*kernel_size+1, 2));

			for (int y=-kernel_size; y<kernel_size+1; y++)
			{
				for (int x=-kernel_size; x<kernel_size+1; x++)
				{
					uint16_t val = src.at<uint16_t>(i+y, j+x);
					if (val != 0)
					{
						target_kernel.push_back(src.at<uint16_t>(i+y, j+x));
					}
				}
			}

			uint16_t median = calc_median(target_kernel);
			uint16_t val = src.at<uint16_t>(i, j);

			if (val == 0) tmp.at<uint16_t>(i, j) = median;
			else
			{
				bool above_threshold = get_above_diff_threshold(val, median, diff_threshold);
				if (above_threshold) tmp.at<uint16_t>(i, j) = median;
				else tmp.at<uint16_t>(i, j) = val;
			}
		}
	}

	dst = tmp;
}

cv::Mat median_filter(cv::Mat& src, int kernel_size=1, float diff_threshold=0.02f)
{	// Overloaded convenience function to return a Mat

	cv::Mat dst;
	median_filter(src, dst, kernel_size, diff_threshold);
	return dst;
}
