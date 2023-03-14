#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>	// This testing framework
#include <fmt/core.h>	      	// formatted terminal output
#include <fmt/color.h>	      	// formatted ANSI terminal output
#include <hue_codec.h>	      	// The header-only hue codec
#include <cmath>				// ceil function
#include "../src/common.h"		// common code


TEST_CASE("test encoder code points")
{
	for (auto i(code_points_bgr.begin()); i!=code_points_bgr.end(); ++i)
	{
		cv::Vec3b bgr = hue_encode_value(i->first);
		CHECK(bgr == i->second);
	}
}

TEST_CASE("test decoder code points")
{
	for (auto i(code_points_bgr.begin()); i!=code_points_bgr.end(); ++i)
	{
		uint16_t decoded_value = hue_decode_value(i->second);
		CHECK(decoded_value == i->first);
	}
}



TEST_CASE("test value encoder against value decoder")
{
	for (uint16_t value=0; value<=HUE_ENCODER_MAX; value++)
	{
		cv::Vec3b bgr = hue_encode_value(value);
		uint16_t decoded_value = hue_decode_value(bgr);

		CHECK(value == decoded_value);
	}
}

TEST_CASE("test HueCodec encode against decode")
{	// Test encoder against decoder using synthetic depth data.

	const int dmin = 0;
	const int dmax = HUE_ENCODER_MAX;
	HueCodec codec(dmin, dmax, 1.0f, false);

	// Create Mat with values from 0 to max encoding value
	const int dim = ceil(sqrt(dmax));
	cv::Mat depth = generate_synthetic_depth(dim, dim, 0, HUE_ENCODER_MAX+1);

	// Hue-encode and hue-decode the synthetic depth data
	cv::Mat decoded = codec.decode(codec.encode(depth));
	for (int i=0; i<depth.rows; i++)
	{
		for (int j=0; j<depth.cols; j++)
		{
			uint16_t vinput  = depth.at<uint16_t>(i, j);
			uint16_t voutput = decoded.at<uint16_t>(i, j);
			CHECK(vinput == voutput);
		}
	}
}
