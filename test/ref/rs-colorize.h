// File sourced from https://github.com/TetsuriSonoda/rs-colorize
// Used here as a reference decoder with corrections.
//
// Original license: MIT License
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#include <cmath>
#include <vector>
#include <opencv2/opencv.hpp>


// convert rgb value to quantization 0-1535 value
unsigned short RGBtoD(unsigned char r, unsigned char g, unsigned char b)
{
	if (r + g + b > 128)
	{
		// conversion from RGB color to quantized depth value
		if (b + g + r < 128)
		{
			return 0;
		}
		else if (r >= g && r >= b)
		{
			if (g >= b)
			{
				return g - b + 1;
			}
			else
			{
				return (g - b) + 1531;
			}
		}
		else if (g >= r && g >= b)
		{
			return b - r + 511;
		}
		else if (b >= g && b >= r)
		{
			return r - g + 1021;
		}
	}

	return 0;
}

void ColorizedDisparityToDepth(float min_depth, float max_depth, float depth_units, cv::Mat& color_mat, cv::Mat& depth_mat)
{
	auto _width = color_mat.size().width;
	auto _height = color_mat.size().height;

	auto in = reinterpret_cast<const unsigned char*>(color_mat.data);
	auto out = reinterpret_cast<unsigned short*>(depth_mat.data);

	auto min_disparity = 1.0f / max_depth;
	auto max_disparity = 1.0f / (min_depth + 0.1);

	//TODO SSE optimize
	float input{};
	for (auto i = 0; i < _height; i++)
		for (auto j = 0; j < _width; j++)
		{
			auto R = *in++;
			auto G = *in++;
			auto B = *in++;

			auto out_value = RGBtoD(R, G, B);

			if (out_value > 0)
			{
				input = min_disparity + (max_disparity - min_disparity) * out_value / 1535.0f;
				*out++ = static_cast<unsigned short>((1.0f / input) / depth_units + 0.5f);
			}
			else
			{
				*out++ = 0;
			}
		}
}

void ColorizedDepthToDepth(float min_depth, float max_depth, float depth_units, cv::Mat& color_mat, cv::Mat& depth_mat)
{
	auto _width = color_mat.size().width;
	auto _height = color_mat.size().height;

	auto in = reinterpret_cast<const unsigned char*>(color_mat.data);
	auto out = reinterpret_cast<unsigned short*>(depth_mat.data);

	//TODO SSE optimize
	for (auto i = 0; i < _height; i++)
	{
		for (auto j = 0; j < _width; j++)
		{
			auto R = *in++;
			auto G = *in++;
			auto B = *in++;

			auto out_value = RGBtoD(R, G, B);

			if(out_value > 0)
			{
				auto z_value = static_cast<unsigned short>((min_depth + (max_depth - min_depth) * out_value / 1535.0f) / depth_units + 0.5f);
				*out++ = z_value;
			}
			else
			{
				*out++ = 0;
			}
		}
	}
}

unsigned short GetMedian(int diff_threshold, std::vector<unsigned short>& target_kernel)
{
	int num_array = target_kernel.size();
	int counter = target_kernel.size();
	int zero_counter = 0;
	int i;
	int v;

	// bubble sort
	for (i = 0; i < num_array; i++)
	{
		for (v = 0; v < num_array - 1; v++)
		{
			if (target_kernel[v] < target_kernel[v + 1])
			{
				if (target_kernel[v] == 0) { zero_counter++; }
				// swap
				int tmp = target_kernel[v + 1];
				target_kernel[v + 1] = target_kernel[v];
				target_kernel[v] = tmp;
			}
		}
		counter = counter - 1;
	}

	if (target_kernel[num_array / 2 + 1] == 0) { return 0; }
	else if (target_kernel[0] - target_kernel[num_array / 2 + 1] > diff_threshold * target_kernel[num_array / 2 + 1] / 1000 || zero_counter > 0) { return 0; }
	else { return target_kernel[num_array / 2 + 1]; }
}

void PostProcessingMedianFilter(int kernel_size, int diff_threshold, cv::Mat& in_depth_mat, cv::Mat& out_depth_mat)
{
	std::vector<unsigned short> target_kernel;
	target_kernel.resize(9);
	out_depth_mat.setTo(0);

	for (int y = kernel_size; y < in_depth_mat.size().height - kernel_size; y++)
	{
		for (int x = kernel_size; x < in_depth_mat.size().width - kernel_size; x++)
		{
			int counter = 0;
			for (int yy = -kernel_size; yy < kernel_size + 1; yy += kernel_size)
			{
				for (int xx = -kernel_size; xx < kernel_size + 1; xx += kernel_size)
				{
					target_kernel[counter] = ((unsigned short*)in_depth_mat.data)[in_depth_mat.size().width * (y + yy) + (x + xx)];
					counter++;
				}
			}

			((unsigned short*)out_depth_mat.data)[in_depth_mat.size().width * y + x] = GetMedian(diff_threshold, target_kernel);
		}
	}
}


void DisparityToDepth(float depth_units, cv::Mat& disparity_mat, cv::Mat& depth_mat)
{
	auto _width = disparity_mat.size().width;
	auto _height = disparity_mat.size().height;

	auto in = reinterpret_cast<const float*>(disparity_mat.data);
	auto out = reinterpret_cast<unsigned short*>(depth_mat.data);

	float input{};
	//TODO SSE optimize
	for (auto i = 0; i < _height; i++)
		for (auto j = 0; j < _width; j++)
		{
			input = *in;
			if (std::isnormal(input))
				*out++ = static_cast<unsigned short>((1.0f / input) / depth_units + 0.5f);
			else
				*out++ = 0;
			in++;
		}
}

