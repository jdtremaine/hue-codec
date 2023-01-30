#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>					// This testing framework
#include <hue_codec.h>							// The header-only hue codec
#include <librealsense2/rs.hpp>					// Include RealSense Cross Platform API
#include <librealsense2/hpp/rs_internal.hpp>	// RealSense software-only device
#include "ref/rs-colorize.h"					// Corrected reference decoder 
#include <fmt/core.h>							// formatted terminal output
#include <fmt/color.h>							// formatted ANSI terminal output
#include "common.h"								// common code


TEST_CASE("test value encoder against corrected reference decoder") 
{	// The source for the reference decoder is the development branch of rs-colorize
	// Compare ref/rs-colorize.cpp to the original to see the changes

	for (uint16_t value=0; value<1530; value++)
	{
		cv::Vec3b bgr = hue_encode_value(value);
		uint16_t ref_decoded_value = RGBtoD(bgr[2], bgr[1], bgr[0]);

		CHECK(value == ref_decoded_value);
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

	auto depth_min_m = 0.0f;
	auto depth_max_m = 1.531f;
	float depth_scale = 0.001f;

    // Initialize the RealSense-native colorizer to Hue colorization
    rs2::colorizer color_map;
	color_map.set_option(RS2_OPTION_HISTOGRAM_EQUALIZATION_ENABLED, 0);
	color_map.set_option(RS2_OPTION_COLOR_SCHEME, 9.0f);		// Hue colorization
	color_map.set_option(RS2_OPTION_MAX_DISTANCE, depth_max_m);
	color_map.set_option(RS2_OPTION_MIN_DISTANCE, depth_min_m);

	// Initialize the hue codec
	HueCodec hc(depth_min_m, depth_max_m, depth_scale, false);

	int W = 40;	// (x)
	int H = 40;	// (y)
	int BPP = 2;

	// Create Mat with values from 0 to 1600 (max encoding value is 1530)
	cv::Mat depth = generate_synthetic_depth(W, H, 0, W*H, 1.0f);

    rs2::software_device dev; // Create software-only device
    auto depth_sensor = dev.add_sensor("Depth"); // Define a single depth sensor

	rs2_intrinsics depth_intrinsics = { 
		W, H, 
		(float)W / 2, (float)H / 2, 
		(float)W, (float)H,
		RS2_DISTORTION_BROWN_CONRADY, { 0,0,0,0,0 } 
	};

    auto depth_stream = depth_sensor.add_video_stream({ 
		RS2_STREAM_DEPTH, 0, 0,
		W, H, 60, BPP,
		RS2_FORMAT_Z16, depth_intrinsics 
	});

	// Compare all streams according to timestamp
    dev.create_matcher( RS2_MATCHER_DEFAULT );  
    rs2::syncer sync;
    depth_sensor.open(depth_stream);
    depth_sensor.start(sync);

	int frame_number = 1;
	rs2_time_t timestamp = (rs2_time_t)frame_number * 16;
    auto domain = RS2_TIMESTAMP_DOMAIN_HARDWARE_CLOCK;

	// Set synthetic depth frame data and parameters
	depth_sensor.on_video_frame({ 
		 (uint16_t*)depth.data,		// Frame pixels from capture API
		 []( void * ) {},           // Custom deleter (if required)
		 W * BPP,					// Stride
		 BPP,
		 timestamp, domain, frame_number,
		 depth_stream, 
		 0.001f						// depth unit
	});  

	rs2::frameset fset = sync.wait_for_frames();
    rs2::frame rs_depth_frame = fset.first_or_default(RS2_STREAM_DEPTH);

	// Use RealSense colorizer to hue-encode depth data
	rs2::frame rs_color_frame = rs_depth_frame.apply_filter(color_map);

	// Create OpenCV matrix from the colorized depth data
	cv::Mat rs_color(H, W, CV_8UC3, (void*)rs_color_frame.get_data());

	// Convert RealSense RGB colorized depth to OpenCV-standard BGR.
	cv::cvtColor(rs_color, rs_color, cv::COLOR_RGB2BGR);

	// Use hue encoder to hue-encode depth data
	cv::Mat hc_color;
	hc.encode(depth, hc_color);

	// Compare the two hue-encoded images
	fmt::print("\nComparing RealSense (\"RS\") and hue codec (\"HC\") RGB values...\n");
	for (int i=0; i<rs_color.rows; i++)
	{
		for (int j=0; j<rs_color.cols; j++)
		{
			auto d = depth.at<uint16_t>(i, j);

			if (d < 1532)
			{
				auto rsc = rs_color.at<cv::Vec3b>(i, j);
				auto hcc = hc_color.at<cv::Vec3b>(i, j);

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

