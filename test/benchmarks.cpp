#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>  				// This testing framework
#include <fmt/color.h>	      				// formatted ANSI terminal output
#include <fmt/core.h>	      				// formatted terminal output
#include <hue_codec.h>	      				// The header-only hue codec
#include <chrono>			  				// performance timing
#include <cstdio>			  				// remove function
#include <ios>				  				// for std::ios_base::bin
#include <opencv2/core/utils/logger.hpp>  	// OpenCV logging controls
#include <opencv2/videoio.hpp>		  		// OpenCV Video input/output
#include "../src/common.h"  				// common code

// All benchmarks use the Peak Signal-to-Noise Ratio (PSNR) as a measure of fidelity/loss.
// [https://en.wikipedia.org/wiki/Peak_signal-to-noise_ratio]
// Standard (uniform) colorization is used as its error is increases linearly with increasing depth.
// As discussed in the Intel whitepaper, PSNR is not a good measure of fidelity for the inverse colorization method.
// This is because its depth quantization error increases quadratically with increasing depth.

using namespace cv;
using namespace std;

struct Performance
{
	float psnr;		// peak signal-to-noise ratio
	int osize;		// original size
	int csize;		// compressed size
	int time_he;	// hue encode
	int time_co;	// compress   / media codec encoding
	int time_de;	// decompress / media codec decoding
	int time_hd;	// hue decode

	int time_total() { return time_he + time_co + time_de + time_hd; }
	int time_save()  { return time_he + time_co; }
	int time_load()  { return time_de + time_hd; }
	float cr()	 	 { return (float)osize / csize; }	// compression ratio
};



Performance image_format_benchmark(const HueCodec& codec, const Mat& depth, const string& file_extension, const vector<int>& params)
{
	using namespace chrono;
	vector<uchar> compressed;
	Mat encoded, decompressed, decoded;

	auto t1 = high_resolution_clock::now();

	// Hue-encode the depth map
	encoded = codec.encode(depth);

	auto t2 = high_resolution_clock::now();

	// Compress the hue-encoded image to a buffer with an image format
	if (file_extension != "")
	{
		imencode("." + file_extension, encoded, compressed, params);
	}

	auto t3 = high_resolution_clock::now();

	// Decompress the hue-encoded image from the buffer
	if (file_extension != "")
	{
		decompressed = imdecode(compressed, IMREAD_COLOR);
	}

	auto t4 = high_resolution_clock::now();

	// Decode the hue-encoded image into a depth map
	if (file_extension != "")
	{
		decoded = codec.decode(decompressed);
	}
	else
	{
		decoded = codec.decode(encoded);
	}

	auto t5 = high_resolution_clock::now();

	// Calculate durations
	int time_he = duration_cast<milliseconds>(t2-t1).count();	// hue encode
	int time_co = duration_cast<milliseconds>(t3-t2).count();	// compress
	int time_de = duration_cast<milliseconds>(t4-t3).count();	// decompress
	int time_hd = duration_cast<milliseconds>(t5-t4).count();	// hue decode

	// Measure the compressed size.
	int csize = compressed.size();
	int osize = 2 * depth.size().area();

	// Calculate the peak signal-to-noise ratio (PSNR) between the original and decoded depth maps
	float psnr = psnr_depth(depth, decoded, codec.depth_max_m(), codec.depth_scale());

	return Performance{psnr, osize, csize, time_he, time_co, time_de, time_hd};
}

void output_image_format_benchmark(const HueCodec& codec, const Mat& depth, string name, string ext, int param_flag, int qmin, int qmax, int qstep)
{
	for (int i=qmin; i!=qmax+qstep; i+=qstep)
	{
		vector<int> params { param_flag, i };
		auto perf = image_format_benchmark(codec, depth, ext, params);
		fmt::print("Hue-encoded {:5}(Q={:>03}) | PSNR: {:5.1f} | CR: {:5.1f} | save: {:>3}ms | load: {:>2}ms\n", name, i, perf.psnr, perf.cr(), perf.time_save(), perf.time_load());
	}
}

TEST_CASE("image psnr test")
{
	const float depth_min_m = 2.2f;
	const float depth_max_m = 7.2f;
	const float depth_scale = HUE_MM_SCALE;
	HueCodec codec(depth_min_m, depth_max_m, depth_scale, false);

	Mat depth = imread("../data/ref/room.png", IMREAD_ANYDEPTH);

	fmt::print("\n{:-<{}}\n", "Image encoding benchmarks on room reference depth map", 80);

	// Test encoding and decoding only
	Performance perf = image_format_benchmark(codec, depth, "", vector<int>());
	fmt::print("Hue encoding only        | PSNR: {:5.1f} | CR: {:5.1f} | save: {:>3}ms | load: {:>2}ms\n", perf.psnr, 1.0f, perf.time_save(), perf.time_load());

	// Test encoding, image format compression, and decoding
	output_image_format_benchmark(codec, depth, "PNG",  "png",  IMWRITE_PNG_COMPRESSION, 10,   1, -1);
	output_image_format_benchmark(codec, depth, "JPEG", "jpg",  IMWRITE_JPEG_QUALITY,     0, 100, 10);
	output_image_format_benchmark(codec, depth, "WebP", "webp", IMWRITE_WEBP_QUALITY,     0, 100, 10);
}

Performance video_format_benchmark(const HueCodec& codec, const vector<Mat>& sequence, const string& ext, const string& fourcc, int api_preference=CAP_FFMPEG)
{
	using namespace chrono;
	VideoWriter vwriter;
	const string video_path = "test_" + fourcc + "." + ext;

	// Initialize the video writer and video file
	if (fourcc != "")
	{
		int video_codec = VideoWriter::fourcc(fourcc[0], fourcc[1], fourcc[2], fourcc[3]);
		vwriter.open(video_path, api_preference, video_codec, 30, sequence.back().size());
		if (!vwriter.isOpened())
		{
			fmt::print("Could not open video file for writing.\n");
			return Performance();
		}
	}

	auto t1 = high_resolution_clock::now();

	// Hue-encode the data
	vector<Mat> encoded;
	for (size_t i=0; i<sequence.size(); i++)
	{
		encoded.push_back(codec.encode(sequence[i]));
	}

	auto t2 = high_resolution_clock::now();

	// Compress / write the video
	if (fourcc != "")
	{
		for (size_t i=0; i<encoded.size(); i++)
		{
			vwriter.write(encoded[i]);
		}
	}

	auto t3 = high_resolution_clock::now();

	VideoCapture vreader;
	if (fourcc != "")
	{
		vwriter.release();
		vreader.open(video_path);
		if (!vreader.isOpened())
		{
			fmt::print("Could not open video file for reading.\n");
			return Performance();
		}
	}

	auto t4 = high_resolution_clock::now();

	// Decompress the video
	vector<Mat> decompressed;
	if (fourcc != "")
	{
		for (size_t i=0; i<sequence.size(); i++)
		{
			// Compare each video frame to the original sequence frame
			Mat frame;
			vreader.read(frame);
			decompressed.push_back(frame);
		}
	}

	auto t5 = high_resolution_clock::now();

	// Decode the video and get the cumulative psnr
	Mat decoded;
	float cum_psnr = 0.0f;
	vector<Mat>* p_encoded_data;
	if (fourcc != "")
	{
		p_encoded_data = &decompressed;
	}
	else
	{
		p_encoded_data = &encoded;
	}

	for (size_t i=0; i<decompressed.size(); i++)
	{
		codec.decode((*p_encoded_data)[i], decoded);
		cum_psnr += psnr_depth(decoded, sequence[i], codec.depth_max_m(), codec.depth_scale());
	}

	auto t6 = high_resolution_clock::now();

	float mean_psnr = cum_psnr / sequence.size();

	// Calculate the original size
	int osize = 2 * sequence.size() * sequence.back().size().area();

	// Get the video file size.
	int csize = 0;
	if (fourcc != "")
	{
		ifstream ifs(video_path, ios_base::binary);
		ifs.seekg(0, ios::end);
		csize = ifs.tellg();
	}

	// Calculate durations
	int time_he = duration_cast<milliseconds>(t2-t1).count();	// hue encode
	int time_co = duration_cast<milliseconds>(t3-t2).count();	// compress
	int time_de = duration_cast<milliseconds>(t5-t4).count();	// decompress
	int time_hd = duration_cast<milliseconds>(t6-t5).count();	// hue decode

	// Delete the video file.
	remove(video_path.c_str());

	return Performance{mean_psnr, osize, csize, time_he, time_co, time_de, time_hd};
}

void output_video_format_benchmark(const HueCodec& codec, const vector<Mat>& sequence, const string& ext, const string& fourcc, int api_preference=CAP_FFMPEG)
{
	auto perf = video_format_benchmark(codec, sequence, ext, fourcc, api_preference);
	fmt::print("Hue-encoded {:<4}/{:<3} | mean PSNR: {:4.1f} | CR: {:>5.1f} | save: {:>3}ms | load: {:>2}ms\n", fourcc, ext, perf.psnr, perf.cr(), perf.time_save(), perf.time_load());
}

TEST_CASE("video psnr test")
{
	const float depth_min_m = 0.8f;
	const float depth_max_m = 5.8f;
	const float depth_scale = HUE_MM_SCALE;
	HueCodec codec(depth_min_m, depth_max_m, depth_scale, false);

	// Load the reference sequence
	vector<Mat> sequence;
	load_reference_sequence("../data/seq/", sequence);

	fmt::print("\n{:-<{}}\n", "Video encoding benchmarks ", 80);

	// Hue encoding and decoding only
	Performance perf = video_format_benchmark(codec, sequence, "", "");
	fmt::print("Hue encoding only    | mean PSNR: {:4.1f} | CR: {:>5.1f} | save: {:>3}ms | load: {:>2}ms\n", perf.psnr, 1.0f, perf.time_save(), perf.time_load());

	// AVI container codecs
	output_video_format_benchmark(codec, sequence, "avi", "MJPG");	// Motion JPEG
	output_video_format_benchmark(codec, sequence, "avi", "XVID");	// Xvid
	output_video_format_benchmark(codec, sequence, "avi", "x264");	// H.264
	output_video_format_benchmark(codec, sequence, "avi", "VP80");	// VP8
	output_video_format_benchmark(codec, sequence, "avi", "VP90");	// VP9

	// MP4 container codecs
	output_video_format_benchmark(codec, sequence, "mp4", "mp4v");	// MPEG video
	output_video_format_benchmark(codec, sequence, "mp4", "avc1");	// H.264 / AVC
	output_video_format_benchmark(codec, sequence, "mp4", "vp09");	// VP9
	output_video_format_benchmark(codec, sequence, "mp4", "hvc1");	// H.265
}

