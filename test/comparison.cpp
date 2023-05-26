#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>					// This testing framework
#include <hue_codec.h>							// The header-only hue codec
#include <librealsense2/rs.hpp>					// Include RealSense Cross Platform API
#include <librealsense2/hpp/rs_internal.hpp>	// RealSense software-only device
#include "ref/rs-colorize.h"					// Corrected reference decoder
#include <memory>								// shared_ptr
#include <fmt/core.h>							// formatted terminal output
#include <fmt/color.h>							// formatted ANSI terminal output
#include "../src/common.h"						// common code

using namespace cv;
using namespace std;

class RSEncoder
{	// A wrapper around the RealSense encoder using a software-only device
	public:
	RSEncoder(int height, int width, float depth_min_m, float depth_max_m, float depth_scale)
	: m_height(height)
	, m_width(width)
	, m_depth_scale(depth_scale)
	, m_frame_number(1)
	{
		// Initialize the RealSense-native colorizer to Hue colorization
		//m_color_map.set_option(RS2_OPTION_HISTOGRAM_EQUALIZATION_ENABLED, 0);
		m_color_map.set_option(RS2_OPTION_HISTOGRAM_EQUALIZATION_ENABLED, 0);
		m_color_map.set_option(RS2_OPTION_COLOR_SCHEME, 9.0f);		// Hue colorization
		m_color_map.set_option(RS2_OPTION_MAX_DISTANCE, depth_max_m);
		m_color_map.set_option(RS2_OPTION_MIN_DISTANCE, depth_min_m);

		auto depth_sensor = m_dev.add_sensor("Depth"); // Define a single depth sensor
		mp_depth_sensor = make_unique<rs2::software_sensor>(depth_sensor);

		rs2_intrinsics depth_intrinsics = {
			m_width, m_height,
			(float)m_width / 2, (float)m_height / 2,
			(float)m_width, (float)m_height,
			RS2_DISTORTION_BROWN_CONRADY, { 0,0,0,0,0 }
		};

		m_depth_stream = mp_depth_sensor->add_video_stream({
			RS2_STREAM_DEPTH, 0, 0,
			m_width, m_height, 60, 2,
			RS2_FORMAT_Z16, depth_intrinsics
		});

		// Compare all streams according to timestamp
		m_dev.create_matcher( RS2_MATCHER_DEFAULT );
		mp_depth_sensor->open(m_depth_stream);
		mp_depth_sensor->start(m_sync);
	}


	void encode(const Mat& src, Mat& dst)
	{
		rs2_time_t timestamp = (rs2_time_t)m_frame_number * 16;
		auto domain = RS2_TIMESTAMP_DOMAIN_HARDWARE_CLOCK;

		// Set synthetic depth frame data and parameters
		mp_depth_sensor->on_video_frame({
			 (uint16_t*)src.data,		// Frame pixels from capture API
			 []( void * ) {},           // Custom deleter (if required)
			 m_width * 2,				// Stride
			 2,
			 timestamp, domain, m_frame_number,
			 m_depth_stream,
			 m_depth_scale
		});

		rs2::frameset fset = m_sync.wait_for_frames();
		m_frame_depth = fset.first_or_default(RS2_STREAM_DEPTH);

		// Use RealSense colorizer to hue-encode depth data
		rs2::frame m_frame_color = m_frame_depth.apply_filter(m_color_map);

		// Create OpenCV matrix from the colorized depth data
		dst = Mat(m_height, m_width, CV_8UC3, (void*)m_frame_color.get_data());

		// Convert RealSense RGB colorized depth to OpenCV-standard BGR.
		cvtColor(dst, dst, COLOR_RGB2BGR);

		// Increment the frame number
		m_frame_number++;
	}

	Mat encode(const Mat& src)
	{
		Mat dst;
		encode(src, dst);
		return dst;
	}


	private:
	rs2::colorizer m_color_map;
	rs2::software_device m_dev;				// software-only device
	unique_ptr<rs2::software_sensor> mp_depth_sensor;	// software-only sensor
	rs2::frame m_frame_depth, m_frame_color;
	rs2::syncer m_sync;
	rs2::stream_profile m_depth_stream;
	int m_height, m_width;
	float m_depth_scale;
	int m_frame_number;
};


TEST_CASE("test value encoder against corrected reference decoder")
{	// The source for the reference decoder is the development branch of rs-colorize
	// Compare ref/rs-colorize.cpp to the original to see the changes

	for (uint16_t value=0; value<HUE_ENCODER_MAX; value++)
	{
		Vec3b bgr = hue_encode_value(value);
		uint16_t ref_decoded_value = RGBtoD(bgr[2], bgr[1], bgr[0]);

		CHECK(value == ref_decoded_value);
	}
}

TEST_CASE("test RealSense encoder code points")
{
	// This test uses synthetic frame data to directly test the RealSense encoder.
	// Unfortunately, there are errors in the output of the RealSense hue encoder.
	// Therefore this test does not actually test any values.
	// Instead it shows the depth value, 
	// and the RealSense ("RS") and code point ("CP") RGB values.

	const float depth_scale = 0.001f;
	const float depth_min_m = 0.0f;
	const float depth_max_m = (HUE_ENCODER_MAX + 1) * depth_scale;
	const int W = 1;
	const int H = code_points_bgr.size();

	RSEncoder rs(H, W, depth_min_m, depth_max_m, depth_scale);

	// Create a Mat with the scaled depth code point values
	Mat depth_values(H, W, CV_16U, Scalar(0));
	for (auto i(code_points_bgr.begin()); i!=code_points_bgr.end(); ++i)
	{
		size_t index = i-code_points_bgr.begin();
		depth_values.at<uint16_t>(index, 0) = i->first;
	}

	// Encode
	Mat bgr_values = rs.encode(depth_values);

	fmt::print("\n{:->{}}", "-", 79);
	fmt::print("\nComparing RealSense (\"RS\") and code point (\"CP\") RGB values...\n");
	for (auto i(code_points_bgr.begin()); i!=code_points_bgr.end(); ++i)
	{
		// Get the code point value and bgr value
		uint16_t  cpv = i->first;
		Vec3b cpc = i->second;

		// The the RealSense bgr value
		size_t index = i-code_points_bgr.begin();
		Vec3b rsc = bgr_values.at<Vec3b>(index, 0);

		// Output the matches and mismatches
		if (rsc == cpc) fmt::print(fg(fmt::color::green),  "   match");
		else            fmt::print(fg(fmt::color::yellow), "mismatch");
		fmt::print(" for d = {:<4}  |  RS:", cpv);
		if (rsc[2] != cpc[2]) fmt::print(fg(fmt::color::red), " {:03}", rsc[2]);
		else fmt::print(" {:03}", rsc[2]);
		if (rsc[1] != cpc[1]) fmt::print(fg(fmt::color::red), " {:03}", rsc[1]);
		else fmt::print(" {:03}", rsc[1]);
		if (rsc[0] != cpc[0]) fmt::print(fg(fmt::color::red), " {:03}", rsc[0]);
		else fmt::print(" {:03}", rsc[0]);
		if (rsc == cpc) fmt::print("  ==  ");
		else fmt::print("  !=  ");
		fmt::print("CP: {:03} {:03} {:03} \n", cpc[2], cpc[1], cpc[0]);
	}
}

TEST_CASE("test RealSense encoder compatibility using synthetic depth data")
{
	// This test uses a synthetic RealSense sensor and synthetic frame data.
	// This allows a direct comparison of the RealSense colorizer and the hue encoder.
	// Unfortunately, there are errors in the output of the RealSense hue encoder.
	// Therefore this test does not actually test any values.
	// Instead it shows value matches and mismatches
	// between the RealSense encoder ("RS") and this hue codec encoder ("HC")

	float depth_scale = 0.001f;
	auto depth_min_m = 0.0f;
	auto depth_max_m = (HUE_ENCODER_MAX + 1) * depth_scale;
	int W = 40;	// (x)
	int H = 40;	// (y)

	// Create Mat with values from 0 to 1600 (max encoding value is 1530)
	Mat depth = generate_synthetic_depth(W, H, 0, W*H, 1.0f);

	// Use the RealSense SDK encoder to hue-encode the depth data
	RSEncoder rs(H, W, depth_min_m, depth_max_m, depth_scale);
	Mat rs_color = rs.encode(depth);

	// Use hue encoder to hue-encode the depth data
	HueCodec hc(depth_min_m, depth_max_m, depth_scale, false); // Initialize the hue codec
	Mat hc_color = hc.encode(depth);

	// Compare the two hue-encoded images
	fmt::print("\n{:->{}}", "-", 79);
	fmt::print("\nComparing RealSense (\"RS\") and hue codec (\"HC\") RGB values...\n");
	for (int i=0; i<rs_color.rows; i++)
	{
		for (int j=0; j<rs_color.cols; j++)
		{
			auto d = depth.at<uint16_t>(i, j);

			if (d < HUE_ENCODER_MAX + 2)
			{
				auto rsc = rs_color.at<Vec3b>(i, j);
				auto hcc = hc_color.at<Vec3b>(i, j);

				// Output the matches and mismatches
				if (rsc == hcc) fmt::print(fg(fmt::color::green),  "   match");
				else            fmt::print(fg(fmt::color::yellow), "mismatch");
				fmt::print(" for d = {:<4}  |  RS:", d);
				if (rsc[2] != hcc[2]) fmt::print(fg(fmt::color::red), " {:03}", rsc[2]);
				else fmt::print(" {:03}", rsc[2]);
				if (rsc[1] != hcc[1]) fmt::print(fg(fmt::color::red), " {:03}", rsc[1]);
				else fmt::print(" {:03}", rsc[1]);
				if (rsc[0] != hcc[0]) fmt::print(fg(fmt::color::red), " {:03}", rsc[0]);
				else fmt::print(" {:03}", rsc[0]);
				if (rsc == hcc) fmt::print("  ==  ");
				else fmt::print("  !=  ");
				fmt::print("HC: {:03} {:03} {:03} \n", hcc[2], hcc[1], hcc[0]);
			}
		}
	}
}


TEST_CASE("compare psnr using the reference sequence")
{
	float depth_min_m = 0.8f;
	float depth_max_m = 5.8f;
	float depth_scale = 0.001f;

	// Load the reference sequence
	vector<Mat> sequence;
	load_reference_sequence("../data/seq/", sequence);

	RSEncoder rs(sequence.back().rows, sequence.back().cols, depth_min_m, depth_max_m, depth_scale);
	HueCodec hc(depth_min_m, depth_max_m, depth_scale, false);

	// Calculate the cumulative psnr for both encoders
	float rs_psnr = 0.0f, hc_psnr = 0.0f;
	for (auto i(sequence.begin()); i!=sequence.end(); ++i)
	{
		// Use the RealSense SDK encoder to hue-encode the depth data
		Mat rs_color = rs.encode(*i);

		// Use hue encoder to hue-encode the depth data
		Mat hc_color = hc.encode(*i);

		// Compare the PSNR of each.
		Mat rs_decoded = hc.decode(rs_color);
		Mat hc_decoded = hc.decode(hc_color);

		// Uncomment below for frame-by-frame visual comparison
		// imshow_depth("rs", rs_decoded);
		// imshow_depth("hc", rs_decoded);
		// waitKey(0);

		rs_psnr += psnr_depth(*i, rs_decoded, depth_max_m, depth_scale);
		hc_psnr += psnr_depth(*i, hc_decoded, depth_max_m, depth_scale);
	}

	// Calculate the mean psnr
	rs_psnr /= (float)sequence.size();
	hc_psnr /= (float)sequence.size();

	fmt::print("\n{:->{}}", "-", 79);
	fmt::print("\nEncoder performance comparison on the reference sequence:\n\n");
	fmt::print("RealSense   hue encoding and decoding mean PSNR:  {:>5.1f}\n", rs_psnr);
	fmt::print("Hue Encoder hue encoding and decoding mean PSNR:  {:>5.1f}\n", hc_psnr);
	fmt::print("\nHue Encoder mean PSNR improvement over RealSense: {:>5.1f}\n", hc_psnr-rs_psnr);
}

