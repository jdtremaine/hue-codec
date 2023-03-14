#include <librealsense2/rs.hpp>
#include <hue_codec.h>
#include "common.h"

// Example demonstrating how to read, hue-encode, save, hue-decode, and then 
// display every frame in a RealSense sensor depth stream.

using namespace cv;
using namespace std;

int main()
{
	// Initialize the hue codec
	float dmin_m = 0.3f;
	float dmax_m = 10.0f;
	float dscale = 0.001f;
	bool inverted = false;
	HueCodec hue_codec(dmin_m, dmax_m, dscale, inverted);

    // Initalize a RealSense pipeline, and start streaming.
	cout << "Initializing the RealSense depth sensor..." << endl;
    rs2::pipeline pipe;
    pipe.start();

	// Get the sensor frame width and height
	rs2::frameset rs_data = pipe.wait_for_frames(); 
	rs2::frame rs_depth_frame = rs_data.get_depth_frame();
	const int w = rs_depth_frame.as<rs2::video_frame>().get_width();
	const int h = rs_depth_frame.as<rs2::video_frame>().get_height();

	// Open the video file
	// Store the video as a motion jpeg
	// Motion jpeg is used here as it is available on all platforms
	// Other codecs will have better compression performance and speed
	string fourcc = "MJPG";		
	int video_codec = VideoWriter::fourcc(fourcc[0], fourcc[1], fourcc[2], fourcc[3]);
	string video_path = "encoded_stream_001.avi";

	cv::VideoWriter vwriter(video_path, video_codec, 30, Size(w, h));
	if (!vwriter.isOpened())
	{
		cerr << "Could not open video file for writing.\n" << endl;
		return EXIT_FAILURE;
	}

	// Record 90 depth frames to a hue-encoded video file.
	cout << "Recording 90 frames to " << video_path << "..." << endl;
	for (int i=0; i<90; i++)
	{
		// Wait for the next set of frames from the camera
		rs_data = pipe.wait_for_frames(); 

		// Get the sensor depth data and initialize an OpenCV Mat using the depth data
		rs_depth_frame = rs_data.get_depth_frame();

		// Wrap the RealSense depth data in an OpenCV Mat
		Mat depth(h, w, CV_16UC1, (void*)rs_depth_frame.get_data());

		// Encode the depth
		Mat encoded = hue_codec.encode(depth);

		// Write the encoded frame to the video file.
		vwriter.write(encoded);
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
	cout << "Playing back the decoded video frames..." << endl;
	Mat encoded_frame;
	while( vreader.read(encoded_frame) )
	{
		Mat decoded = hue_codec.decode(encoded_frame);
		imshow_depth("depth", decoded, dmin_m, dmax_m, dscale);
		char key = waitKey(1);
		if (key == 'q' || key == 27) break;
	}

}


