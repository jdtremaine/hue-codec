#include <opencv2/opencv.hpp>   // Include OpenCV API

// Used by tests and validation
// Note that these code points are in OpenCV-standard BGR format
const std::vector<std::pair<uint16_t, cv::Vec3b>> code_points_bgr {
	{    0, {   0,   0,   0}},
	{    1, {   0,   0, 255}},
	{  256, {   0, 255, 255}},
	{  511, {   0, 255,   0}},
	{  766, { 255, 255,   0}},
	{ 1021, { 255,   0,   0}},
	{ 1276, { 255,   0, 255}},
};

float minmax_clamp(float value, float lower, float upper)
{
	return std::max(lower, std::min(value, upper));
}

cv::Mat render_depth(const cv::Mat& src, float depth_min_m=0.3f, float depth_max_m=10.0f, float depth_scale=0.001f, bool equalize=true)
{
	// Scale the 16-bit depth range into an 8-bit grayscale image and then display it.
	float depth_min_u = depth_min_m / depth_scale;
	float depth_max_u = depth_max_m / depth_scale;

	cv::Mat dst(src.rows, src.cols, CV_8UC1, cv::Scalar(0));
	for (int i=0; i<src.rows; i++)
	{
		for (int j=0; j<src.cols; j++)
		{
			float d = src.at<uint16_t>(i, j);
			float scaled = (d-depth_min_u) / (depth_max_u - depth_min_u);

			uint16_t v = 0;
			if (scaled > 0) v = 255 - round(255.0f * minmax_clamp(scaled, 0.0f, 1.0f));
			dst.at<uint8_t>(i, j) = v;
		}
	}

	// Equalize the histogram for better intelligibility
	if (equalize) equalizeHist(dst, dst);

	return dst;
}

void imshow_depth(const std::string& name, const cv::Mat& src, float depth_min_m=0.3f, float depth_max_m=10.0f, float depth_scale=0.001f, bool equalize=true)
{
	// Scale the 16-bit depth range into an 8-bit grayscale image and then display it.
	cv::Mat dst = render_depth(src, depth_min_m, depth_max_m, depth_scale, equalize);

	// Display
	cv::imshow(name, dst);
}


float psnr_depth(const cv::Mat& a, const cv::Mat& b, float depth_max_m, float depth_scale)
{
	// Calculate the Peak-Signal to Noise Ratio (PSNR) of a depth map
	if (a.empty() || b.empty() || a.size() != b.size()) return -1.0f;
	if (a.type() != CV_16U || b.type() != CV_16U) return -1.0f;

	// Calculate the maximum intensity
	int max_i = depth_max_m / depth_scale;

	// Calculate the MSE
	float mse = 0.0f;
	int count = 0;
	for (int i=0; i<a.rows; i++)
	{
		for (int j=0; j<a.cols; j++)
		{
			int aval = a.at<uint16_t>(i, j);
			int bval = b.at<uint16_t>(i, j);

			if (aval < max_i && bval < max_i)
			{	// Only consider values below max_i
				int diff = aval - bval;
				mse += diff * diff;
				count++;
			}
		}
	}
	if (count == 0) mse = 0;
	else mse = mse / count;

	// Calculate and return the PSNR
	return 20.0f * log10(max_i) - 10.0f * log10(mse);
}


cv::Mat generate_synthetic_depth(int w, int h, float vmin, float vmax, float inc=0.0f)
{
	cv::Mat data(h, w, CV_16U, cv::Scalar(0));
	if (inc == 0.0f) inc = (vmax-vmin) / (w*h);
	int count = 0;
	for (int i=0; i<data.rows; i++)
	{
		for (int j=0; j<data.cols; j++)
		{
			float value = round(vmin + inc*count);
			data.at<uint16_t>(i, j) = round(value);
			count++;
		}
	}

	return data;
}

void load_reference_sequence(std::vector<cv::Mat>& sequence, std::string seq_path="../data/seq/")
{
	// Read the sequence into memory
	sequence.clear();
	const int frame_count = 26;
	for (int i=0; i<frame_count; i++)
	{
		std::stringstream ss_path;
		ss_path << "frame_" << std::setfill('0') << std::setw(5) << i << ".png";
		std::string path = seq_path + ss_path.str();
		sequence.push_back( imread(path, cv::IMREAD_ANYDEPTH) );
	}
}
