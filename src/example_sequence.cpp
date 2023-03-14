#include <hue_codec.h>
#include "common.h"

// Example demonstrating how to read, hue-encode, save, hue-decode, and 
// display a depth sequence.

using namespace cv;
using namespace std;

int main()
{
	// Load the reference sequence
	vector<Mat> sequence;
	load_reference_sequence(sequence);

	// Initialize the hue codec
	float dmin_m = 0.9f;
	float dmax_m = 5.8f;
	float dscale = 0.001f;
	bool inverted = false;
	HueCodec hue_codec(dmin_m, dmax_m, dscale, inverted);

	// Open the video file
	// Store the video as a motion jpeg
	// Motion jpeg is used here as it is available on all platforms
	// Other codecs will have better compression performance and speed
	string fourcc = "MJPG";		
	int video_codec = VideoWriter::fourcc(fourcc[0], fourcc[1], fourcc[2], fourcc[3]);
	string video_path = "encoded_sequence_001.avi";
	cv::VideoWriter vwriter(video_path, video_codec, 30, sequence.back().size());
	if (!vwriter.isOpened())
	{
		cerr << "Could not open video file for writing.\n" << endl;
		return EXIT_FAILURE;
	}

	// Encode the sequence frame-by-frame and write it to the video file
	for (auto i(sequence.begin()); i!=sequence.end(); ++i)
	{
		Mat encoded_frame = hue_codec.encode(*i);
		vwriter.write(encoded_frame);
	}
	vwriter.release();

	// Do other things...

	// Open the video file for reading
	VideoCapture vreader;
	vreader.open(video_path);
	if (!vreader.isOpened())
	{
		cerr << "Could not open video file for reading.\n" << endl;
		return EXIT_FAILURE;
	}
	
	// Read the sequence back from the video file and decode it
	// Display each frame as it is decoded
	Mat encoded_frame;
	while( vreader.read(encoded_frame) )
	{
		Mat decoded = hue_codec.decode(encoded_frame);
		imshow_depth("depth", decoded, dmin_m, dmax_m, dscale);
		waitKey(100);
	}

}


