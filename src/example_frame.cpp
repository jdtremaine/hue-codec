#include <hue_codec.h>
#include "common.h"

// Example demonstrating how to read, hue-encode, save, hue-decode, and 
// display a single depth frame.

using namespace cv;
using namespace std;

int main()
{
	// Open a depth frame (use ANYDEPTH to read a 16-bit PNG)
	Mat depth = imread("../data/ref/table.png", cv::IMREAD_ANYDEPTH);

	// Initialize the hue codec
	float dmin_m = 0.8f;	// Minimum sensor depth in metres               
	float dmax_m = 3.0f;  	// Maximum sensor depth in metres    
	float dscale = 0.001f;	// Depth map 16-bit integer values to metres)
	bool inverted = false;	// Use standard colourization (see documentation)
	HueCodec hue_codec(dmin_m, dmax_m, dscale, inverted);

	// Encode the depth frame to an RGB image
	Mat encoded = hue_codec.encode(depth);

	// Save the image to a jpeg file (quality = 80.
	vector<int> jpg_params { IMWRITE_JPEG_QUALITY, 80};
	imwrite("encoded_frame.jpg",  encoded, jpg_params);

	// Do other things...

	// Read the image back from the jpeg file
	Mat retrieved = imread("encoded_frame.jpg");

	// Decode the RGB image into a depth frame
	Mat decoded = hue_codec.decode(retrieved);

	// Show the original depth frame and the decoded depth frame.
	imshow_depth("original depth", depth,   dmin_m, dmax_m, dscale);
	imshow("encoded depth", encoded);
	imshow_depth("decoded depth",  decoded, dmin_m, dmax_m, dscale);
	waitKey(0);
}


