#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <hue_codec.h>							// The header-only hue codec
#include <fmt/core.h>
#include <doctest/doctest.h>					// This testing framework
#include <opencv2/core/utils/logger.hpp>		// OpenCV logging controls
#include "../src/common.h"						// common code

using namespace cv;
using namespace std;

TEST_CASE("reference sequence viewer") 
{	// Visualization of the reference sequence 
	// Interactively modify depth_min, depth_max, and inverse colourization

	// Disable OpenCV logging
	utils::logging::setLogLevel(utils::logging::LogLevel::LOG_LEVEL_SILENT);

	float dmin_m = 0.9f;
	float dmax_m = 5.8f;
	float dscale = HUE_MM_SCALE;
	bool inverted = false;

	// Load the reference sequence
	vector<Mat> sequence;
	load_reference_sequence(sequence);

	fmt::print("This tool provides an interactive visualization of the hue encoding\n"); 
	fmt::print("by looping over the reference sequence.\n\n"); 
	fmt::print("With one of the image windows selected:\n"); 
	fmt::print("- press the I key to invert the colourization.\n"); 
	fmt::print("- press the H key to decrease depth_min.\n"); 
	fmt::print("- press the L key to increase depth_min.\n"); 
	fmt::print("- press the J key to decrease depth_max.\n"); 
	fmt::print("- press the K key to increase depth_max.\n"); 
	fmt::print("- press the Q key or the ESC key to exit.\n"); 
	fmt::print("- press the space bar to advance to the next frame.\n"); 

	// Calculate the cumulative psnr for both encoders
	for (auto i(sequence.begin());;)
	{
		if (i == sequence.end()) i=sequence.begin(); 

		// Use hue encoder to hue-encode the depth data
		HueCodec hc(dmin_m, dmax_m, dscale, inverted); // Initialize the hue codec
		Mat encoded = hc.encode(*i);

		imshow_depth("depth", *i, dmin_m, dmax_m, dscale);
		imshow("hue-encoded depth", encoded);
		char key  = waitKey(0);
		if (key == 'q' || key == 27) break;
		else if (key == 'j' || key == 'k') 
		{
			if (key == 'j') dmax_m -= 0.1f;
			if (key == 'k') dmax_m += 0.1f;
			fmt::print("depth max  {:>5.1f}\n", dmax_m);
		}
		else if (key == 'h' || key == 'l')
		{
			if (key == 'h') dmin_m -= 0.1f;
			if (key == 'l') dmin_m += 0.1f;
			fmt::print("depth min  {:>5.1f}\n", dmin_m);
		}
		else if (key == 'i') inverted = !inverted;
		else if (key == ' ') ++i;
	}
	destroyAllWindows();
}

TEST_CASE("median filter visualizer")
{	// Visualization of the median filter 
	// Interactively modify the kernel size and difference threshold

	// Disable OpenCV logging
	utils::logging::setLogLevel(utils::logging::LogLevel::LOG_LEVEL_SILENT);

	// Load the reference sequence
	vector<Mat> sequence;
	load_reference_sequence(sequence);

	// Reference sequence depth parameters
	float dmin_m =  0.9f;
	float dmax_m = 5.8f;
	float dscale = HUE_MM_SCALE;

	// Median filter settings
	Mat median_filtered;
	int kernel_size = 1;
	float diff_threshold = 0.0f;
	bool process = true;

	// Inform the user
	fmt::print("\nThis tool provides an interactive visualization of the median filter\n"); 
	fmt::print("by looping over the reference sequence.\n\n"); 
	fmt::print("With one of the image windows selected:\n"); 
	fmt::print("- press h to decrease the kernel size by 1 pixel\n"); 
	fmt::print("- press l to increase the kernel size by 1 pixel\n"); 
	fmt::print("- press j to decrease the threshold by 1 percent\n"); 
	fmt::print("- press k to increase the threshold by 1 percent\n"); 
	fmt::print("- press the space bar to advance to the next frame\n"); 
	fmt::print("- press the Q key or ESC key to exit\n"); 

	for (size_t i = 0;;)
	{
		if (i == sequence.size()) i = 0;

		if (process)
		{
			// Avoid re-processing on every loop
			median_filter(sequence[i], median_filtered, kernel_size, diff_threshold);
			process = false;
		}

		imshow_depth("source depth", sequence[i], dmin_m, dmax_m, dscale);
		imshow_depth("median filtered", median_filtered);

		char key = waitKey(1);
		if (key == 'q' || key == 'Q' || key == 27) break;
		else if (key == 'l')
		{
			kernel_size += 1;
			fmt::print("kernel size: {:>3}\n", kernel_size);
			process = true;
		}
		else if (key == 'h')
		{
			kernel_size -= 1;
			if (kernel_size < 0) kernel_size = 0;
			fmt::print("kernel size: {:>3}\n", kernel_size);
			process = true;
		}
		else if (key == 'j')
		{
			diff_threshold -= 0.005f;
			if (diff_threshold < 0.0f) diff_threshold = 0.0f;
			fmt::print("diff threshold: {:>3}\n", diff_threshold);
			process = true;
		}
		else if (key == 'k')
		{
			diff_threshold += 0.005f;
			fmt::print("diff threshold: {:>3}\n", diff_threshold);
			process = true;
		}
		else if (key == 'w')
		{
			fmt::print("writing depth-rendered 8-bit PNGs...\n");
			vector<int> png_params { IMWRITE_PNG_COMPRESSION, 9 };
			Mat rdepth = render_depth(sequence[i]);
			Mat rmedian_filtered = render_depth(median_filtered);
			imwrite("depth.png", rdepth, png_params);
			imwrite("median_filtered.png", rmedian_filtered, png_params);
		}
		else if (key == ' ')
		{
			i++;
			process = true;
		}
	}
	destroyAllWindows();
}
