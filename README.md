# Table of Contents
- [What is this?](#what-is-this)
- [Why would I use this?](#why-would-i-use-this)
- [How does it work?](#how-does-it-work)
- [What performance can I expect?](#what-performance-can-i-expect)
- [How do I use this?](#how-do-i-use-this)
- [Flying pixels and median filtering](#flying-pixels-and-median-filtering)
- [Inverted colourization](#inverted-colourization)
- [What is the hue encoding scheme?](#what-is-the-hue-encoding-scheme)
- [Comparison to the RealSense encoder and decoder](#comparison-to-the-realsense-encoder-and-decoder)
- [What is included in this repository?](#what-is-included-in-this-repository)
- [How do I get the tests and examples working?](#how-do-i-get-the-tests-and-examples-working)


# What is this?
Hue Codec is a header-only C++ library that efficiently encodes and decodes 16-bit, single-channel bitmaps (i.e. depth maps, disparity maps, infrared images, or high bit-depth grayscale images) to and from 8-bit, 3-channel bitmaps (i.e. RGB images).

Once in RGB format, conventional lossy RGB image and video codecs can achieve high compression ratios and relatively low signal loss on the encoded data. This approach has the advantage of being simple, fast, and portable, and it allows data inspection and previewing using commonly available image and video viewers. This library uses OpenCV for basic image manipulation but also includes a library-independent encoder and decoder for those that want to use other libraries.

This work builds on a [whitepaper](https://dev.intelrealsense.com/docs/depth-image-compression-by-colorization-for-intel-realsense-depth-cameras) and [reference code](https://github.com/TetsuriSonoda/rs-colorize) published by Intel. Note that the whitepaper has several errors, and neither the encoder included in the RealSense SDK nor the reference decoder provided along with the whitepaper is correct. The publication of this codec allows it to be used with other sensors not supported by the Intel RealSense SDK.


# Why would I use this?
This library may be helpful to you if you are trying to stream or save the output from a depth sensor or another type of sensor that outputs 16-bit grayscale images.

Depth sensors generate large amounts of data. For instance, the Intel RealSense D400 series depth sensors can output 16-bit, 848x480 pixel depth maps at 90 frames per second
\[[D400 Datasheet](https://www.intelrealsense.com/wp-content/uploads/2022/11/Intel-RealSense-D400-Series-Datasheet-November-2022.pdf)\].
That's a data rate of 73MB/s (586Mbps) or over 4 gigabytes of raw sensor data per minute.

This codec allows you to encode that data to RGB so that conventional image and video codecs may be used to achieve large compression ratios. This technique lets you use hardware-accelerated image and video codecs to achieve high data throughputs.

The naive approach to encoding 16-bit depth data as RGB data would be to scale the 16-bit value to be a 24-bit value and use that as the RGB pixel value. Unfortunately, all modern lossy image compression techniques use perceptual compression that causes losses unevenly across the RGB image colour space. The result of data loss in the most significant bits will cause severe artifacts in the recovered depth maps.

A better approach would be to encode the 16 bits of depth data as hue information, as colour information is well-preserved by modern lossy image and video codecs. This codec transforms the depth values to hues in the RGB colour space, making encoding and decoding fast, simple, and relatively robust to artifacts.


# How does it work?

A single-frame version of the pipeline works like this:

| Step       | Description                           | Result |
| ----       | -----------                           | ------ |
| Read       | Get a depth frame from a depth sensor | ![Depth Map](/docs/assets/pipeline/01_depth.png "Depth Map") |
| Encode     | Hue-encode the depth frame            | ![Hue-Encoded Image](/docs/assets/pipeline/02_encoded_std.png "Hue-encoded Depth Map") |
| Compress   | Save the encoded image to a file      | webp file (quality=50%)|
| Decompress | Read the image from the file          | ![Hue-Encoded Image](/docs/assets/pipeline/04_decompress_webp050.png "Compressed Hue-encoded Depth Map") |
| Decode     | Decode the depth frame                | ![Decoded Depth Map](/docs/assets/pipeline/05_decoded_webp050.png "Decoded Depth Map") |

In this example, the raw depth data is 115kb.
The raw depth data saved as a 16-bit grayscale PNG is 63kb in size - a compression ratio of 1.8.
The hue-encoded webp file (quality=50%) is 9kb in size - a compression ratio of 13.2.

A series of depth frames is called a depth stream. Depth streams can be saved using standard video codecs like H.264 and H.265.

# What performance can I expect?

Use the benchmarks to get compression rates and frame encoding/decoding times for your data and hardware. Note that these benchmarks only include hue encoding and decoding, image compression and decompression, and do not include saving the data to disk.

The output of the benchmarks test for an Ubuntu system with an AMD Ryzen 7 2700X CPU and an Nvidia GeForce RTX 3080 is below. For single frames, the JPEG format offers the fastest combined save and load times, while WebP offers the fastest load times and best compression ratios while maintaining fidelity. A compression ratio of 60X can be achieved using WebP image compression without significant compression artifacts.

## Image encoding benchmarks
| Encoding                 | PSNR  | CR    | save (ms) | load (ms) | save (kB/s) | load (kB/s) |
| ------------------------ | ----- | ----- | --------- | --------- | ----------- | ----------- |
| Hue-encoded only (Q=100) |  77.4 |   1.0 |       4.5 |       3.0 |        68.3 |       100.7 |
| Hue-encoded PNG  (Q= 10) |  77.4 |   4.8 |     548.2 |       4.7 |         0.6 |        65.8 |
| Hue-encoded PNG  (Q=  9) |  77.4 |   4.8 |     533.7 |       4.6 |         0.6 |        66.3 |
| Hue-encoded PNG  (Q=  8) |  77.4 |   4.7 |     234.4 |       4.9 |         1.3 |        63.3 |
| Hue-encoded PNG  (Q=  7) |  77.4 |   4.6 |      76.5 |       4.8 |         4.0 |        63.7 |
| Hue-encoded PNG  (Q=  6) |  77.4 |   4.5 |      47.1 |       4.8 |         6.5 |        63.4 |
| Hue-encoded PNG  (Q=  5) |  77.4 |   4.3 |      27.6 |       5.1 |        11.1 |        59.8 |
| Hue-encoded PNG  (Q=  4) |  77.4 |   4.1 |      20.0 |       5.3 |        15.4 |        58.2 |
| Hue-encoded PNG  (Q=  3) |  77.4 |   3.8 |      22.5 |       6.8 |        13.7 |        45.0 |
| Hue-encoded PNG  (Q=  2) |  77.4 |   3.5 |      16.9 |       5.4 |        18.2 |        57.3 |
| Hue-encoded PNG  (Q=  1) |  77.4 |   3.3 |      16.2 |       5.5 |        19.0 |        55.8 |
| Hue-encoded JPEG (Q=  0) |  27.5 |  97.7 |       3.3 |       2.8 |        91.8 |       111.4 |
| Hue-encoded JPEG (Q= 10) |  31.3 |  66.8 |       3.5 |       2.9 |        88.7 |       106.1 |
| Hue-encoded JPEG (Q= 20) |  35.1 |  47.7 |       3.7 |       2.9 |        82.3 |       105.5 |
| Hue-encoded JPEG (Q= 30) |  37.8 |  38.5 |       3.5 |       2.9 |        88.3 |       107.2 |
| Hue-encoded JPEG (Q= 40) |  39.7 |  33.1 |       3.5 |       3.0 |        87.5 |       100.9 |
| Hue-encoded JPEG (Q= 50) |  43.3 |  29.1 |       3.6 |       2.9 |        86.4 |       105.7 |
| Hue-encoded JPEG (Q= 60) |  43.3 |  25.7 |       3.6 |       3.6 |        86.1 |        85.8 |
| Hue-encoded JPEG (Q= 70) |  44.8 |  21.9 |       3.6 |       3.0 |        85.0 |       102.9 |
| Hue-encoded JPEG (Q= 80) |  47.7 |  17.5 |       3.6 |       3.0 |        85.8 |       102.5 |
| Hue-encoded JPEG (Q= 90) |  52.8 |  12.0 |       3.7 |       3.5 |        83.4 |        88.8 |
| Hue-encoded JPEG (Q=100) |  56.5 |   3.5 |       4.3 |       4.7 |        72.1 |        66.0 |
| Hue-encoded WebP (Q=  0) |  36.4 | 121.2 |      17.7 |       3.0 |        17.3 |       103.4 |
| Hue-encoded WebP (Q= 10) |  39.5 |  82.1 |      19.7 |       2.9 |        15.6 |       104.3 |
| Hue-encoded WebP (Q= 20) |  40.2 |  66.0 |      20.0 |       3.1 |        15.4 |        97.8 |
| Hue-encoded WebP (Q= 30) |  42.2 |  56.2 |      21.4 |       3.2 |        14.3 |        95.9 |
| Hue-encoded WebP (Q= 40) |  44.7 |  49.5 |      20.5 |       3.3 |        15.0 |        94.5 |
| Hue-encoded WebP (Q= 50) |  44.7 |  44.4 |      21.6 |       3.3 |        14.2 |        92.0 |
| Hue-encoded WebP (Q= 60) |  45.0 |  39.9 |      21.8 |       3.4 |        14.1 |        89.5 |
| Hue-encoded WebP (Q= 70) |  47.0 |  35.9 |      23.0 |       3.8 |        13.3 |        81.4 |
| Hue-encoded WebP (Q= 80) |  50.8 |  29.1 |      27.6 |       3.8 |        11.1 |        81.8 |
| Hue-encoded WebP (Q= 90) |  54.0 |  19.3 |      30.7 |       4.2 |        10.0 |        74.0 |
| Hue-encoded WebP (Q=100) |  55.4 |   8.3 |      37.7 |       6.5 |         8.1 |        46.9 |



## Video encoding benchmarks

On the reference system, hue encoding of depth streams adds an overhead of 0.5s per frame, and hue decoding adds a negligible overhead. Codecs are shown below in the format [fourcc code/container format]. All codecs achieved roughly the same PSNR when the default settings were used. The x264 (a.k.a. avc1) codec represents the best speed and compression ratio combination.

Here are the video benchmarks when ffmpeg is built with vpx, x264, and x265 codecs with CUDA support:

| Encoding             | mean PSNR | CR    | save (ms) | load (ms) | save (kB/s) | load (kB/s) |
| -------------------- | --------- | ----- | --------- | --------- | ----------- | ----------- |
| Hue-encoded only/    |       0.0 |   1.0 |       0.5 |       0.0 |       247.6 |   9950831.0 |
| Hue-encoded MJPG/avi |      28.7 |   6.8 |       1.4 |       0.6 |        83.2 |       180.7 |
| Hue-encoded XVID/avi |      29.1 |   6.3 |       1.0 |       0.6 |       116.2 |       202.3 |
| Hue-encoded x264/avi |      28.0 |  20.9 |       0.6 |       0.9 |       180.8 |       126.9 |
| Hue-encoded VP80/avi |      29.6 |   7.4 |      19.4 |       0.8 |         5.9 |       138.8 |
| Hue-encoded VP90/avi |      29.1 |  13.2 |       5.3 |       0.9 |        21.8 |       128.6 |
| Hue-encoded mp4v/mp4 |      29.1 |   6.3 |       0.9 |       0.5 |       128.6 |       232.1 |
| Hue-encoded avc1/mp4 |      28.0 |  21.7 |       0.9 |       0.9 |       134.7 |       130.9 |
| Hue-encoded vp09/mp4 |      29.1 |  13.6 |       6.1 |       1.1 |        18.8 |       107.5 |
| Hue-encoded hvc1/mp4 |      29.8 |   5.7 |       0.6 |       1.8 |       190.5 |        62.5 |


Here are the video benchmarks when ffmpeg is built with nvcodec:

| Encoding             | mean PSNR | CR    | save (ms) | load (ms) | save (kB/s) | load (kB/s) |
| -------------------- | --------- | ----- | --------- | --------- | ----------- | ----------- |
| Hue-encoded x264/avi |      28.0 |  14.6 |       0.6 |       0.6 |       202.4 |       178.7 |
| Hue-encoded avc1/mp4 |      28.0 |  15.0 |       0.7 |       0.7 |       162.8 |       169.1 |
| Hue-encoded hvc1/mp4 |      29.1 |   9.6 |       0.5 |       1.0 |       220.2 |       112.5 |

While the nvcodec codecs offer marginally higher save and load rates, they have lower compression ratios.


# How do I use this?
The hue\_codec.h file is a header-only library with an external dependency on OpenCV.

You must add the include/hue\_codec.h file to your project.
You will also need to add OpenCV to your project.
How you do this depends on your compiler and setup, but this project includes cross-platform build support using CMake to manage the build and vcpkg to install the dependencies.

Once you have added the hue\_codec.h file into your project,
Add an include directive into your code like so:

    #include "hue_codec.h"

To use the Hue Codec:

This code expects you to use 16-bit depth maps, meaning your depth data will be encoded as unsigned 16-bit integers. Some known scaling factor will be used to convert depth map values into real-world distances. For Intel Depth Sensors, a scaling factor of 0.001 is used to convert between depth map values and metres.

Initialize the codec as so:

    const int min_sensor_depth_m =  0.3f;    // Minimum sensor depth in metres
    const int max_sensor_depth_m = 10.0f;    // Maximum sensor depth in metres
    const float depth_scale      = 0.001f;   // Depth map 16-bit integer values to metres)
    bool inverted = false;                   // Use standard colourization (explained later)
    HueCodec codec(min_sensor_depth_m, max_sensor_depth_m, scaling_factor, inverted);

During hue encoding, the entire depth field is reduced to 1530 values in a lossy way. To preserve as much fidelity as possible, initialize HueCodec objects using depth\_min and depth\_max values that accurately represent the depth range of all your data.

You would then retrieve a 16-bit depth frame from your sensor.
To encode that depth frame, you would use the following:

    cv::Mat encoded_frame = codec.encode(depth_frame);

The OpenCV Mat encoded\_frame is a standard 3-channel, 8-bit BGR image (OpenCV uses BGR instead of RGB).
You can now write the encoded frame with a standard RGB image or video codec and write it to disk.
An example of this would be:

    cv::imwrite("compressed.jpg", encoded_frame);

Later, you can read the image or video file from the disk.
An example to read the file saved above would be:

    cv::Mat encoded_frame = cv::imread("compressed.jpg");

And then decode it like so:

    cv::Mat decoded_frame = codec.decode(encoded_frame);

If lossy compression is used, the OpenCV Mat decoded\_frame will lose fidelity from compression artifacts.


# Flying pixels and median filtering
Compression artifacts in the compressed RGB image can result in a "flying pixel" artifact when decoded. Flying pixels are individual pixels that are much closer to the foreground than they should be. These flying pixels can be cleaned up by postprocessing with a median filter as below.

	int kernel_size = 1;
    float diff_threshold = 0.10;
    cv::Mat cleaned_depth_frame = median_filter(decoded_frame, kernel_size, diff_threshold);


The median filter takes two parameters: a kernel size and a difference threshold.
The kernel size is the box size in pixels around the current pixel that will be used to calculate the median. If a kernel size of 1 is specified, the median will be calculated with a box that spans from 1 pixel up and left to 1 pixel below and right of the current pixel.
The difference threshold is a percentage difference (pixel value - median)/median above which the pixel will be replaced with the median.
So a difference threshold of zero will replace all pixels with their local median.


See below for a comparison of median filter results for different kernel sizes and difference thresholds.


| threshold | kernel size = 1                    | kernel size = 4                    |
| --------- | ---------------------------------- | ---------------------------------- |
| 0.00      | ![k1t00](/docs/assets/median/k1t00.png) | ![k4t00](/docs/assets/median/k4t00.png) |
| 0.05      | ![k1t05](/docs/assets/median/k1t05.png) | ![k4t05](/docs/assets/median/k4t05.png) |
| 0.10      | ![k1t10](/docs/assets/median/k1t10.png) | ![k4t10](/docs/assets/median/k4t10.png) |

As shown above, the difference threshold will also fill zero-valued pixels with the local median value. More information about flying pixels can be found in the [Intel whitepaper](https://dev.intelrealsense.com/docs/depth-image-compression-by-colorization-for-intel-realsense-depth-cameras).


# Inverted colourization
Standard colourization scales values to the encoding value range. The depth measurement error of depth sensors scales quadratically with distance \[[Intel Sensor Tuning Guide](https://dev.intelrealsense.com/docs/tuning-depth-cameras-for-best-performance#section-verify-performance-regularly-on-a-flat-wall-or-target)\]. To match this, it makes sense to scale the inverse of the depth value to the encoding value range instead. This variation of hue encoding is known as inverse colourization; an example is shown below.

| Encoding Type          | Result                                                    |
| ---------------------- | --------------------------------------------------------- |
| Standard colourization | ![Hue-Encoded Image](/docs/assets/pipeline/02_encoded_std.png) |
| Inverse colourization  | ![Hue-Encoded Image](/docs/assets/pipeline/02_encoded_inv.png) |

Note that the background detail is better preserved in the standard encoding, while foreground detail is better preserved in the inverted encoding. More detail about inverted colourization is provided in the [Intel whitepaper](https://dev.intelrealsense.com/docs/depth-image-compression-by-colorization-for-intel-realsense-depth-cameras).


# What is the hue encoding scheme?
The hue encoding method uses a simple, formulaic mapping of values to RGB values in the hue colour space. The mapping covers 1,531 distinct RGB values. Note that a range of 1,531 different values represents about 11 bits of information, so the 16-bit depth values must be scaled to values in the [0-1530] range. You might expect this to incur some data loss, but fortunately, most depth sensors do not have sufficient resolution and range coverage to require all 16 bits of data.

The hue encoding scheme is as follows:

|    value    | red    | green  | blue   | description                |
| ----------- | ------ | ------ | -------| -------------------------- |
|           0 | 0      | 0      | 0      | black                      |
|           1 | 255    | 0      | 0      | red                        |
|    2 -  255 | 255    | v-1    | 0      | red with green ascending   |
|         256 | 255    | 255    | 0      | red + green = yellow       |
|  257 -  510 | 511-v  | 255    | 0      | green with red descending  |
|         511 | 0      | 255    | 0      | green                      |
|  512 -  765 | 0      | 255    | v-511  | green with blue ascending  |
|         766 | 0      | 255    | 255    | green + blue = cyan        |
|  767 - 1020 | 0      | 1021-v | 255    | blue with green descending |
|        1021 | 0      | 0      | 255    | blue                       |
| 1022 - 1275 | v-1021 | 0      | 255    | blue with red ascending    |
|        1276 | 255    | 0      | 255    | blue + red = purple        |
| 1277 - 1530 | 255    | 0      | 1531-v | red with blue descending   |

A complete mapping of values to RGB values is included in [docs/full\_mapping.csv](/docs/full_mapping.csv).


# Comparison to the RealSense encoder and decoder
This implementation's encoding scheme matches the 1531-point encoding scheme described in the Intel whitepaper, adding a zero value mapping to an all-black RGB value as in the RealSense hue encoder.

The errors in the RealSense SDK encoder cause a mean 10.5dB drop in [peak signal-to-noise ratio (PSNR)](https://en.wikipedia.org/wiki/Peak_signal-to-noise_ratio) on the included reference sequence relative to this encoder.

For further examination of the errors in the Intel RealSense SDK encoder and reference decoder, see the comparison test. A copy of the output of the comparison test is included in [docs/comparison\_output.txt](/docs/comparison_output.txt).

# What is included in this repository?

| directory | description                      |
| --------- | -------------------------------- |
| data      | reference depth data for testing |
| env       | platform-specific setup scripts  |
| docs      | documentation                    |
| include   | the hue\_codec.h header file     |
| src       | basic examples                   |
| test      | tests and interactive tools      |


# How do I get the tests and examples working?
If you don't want to run the tests and examples and only want to use this library in your project, see the usage instructions above under [How do I use this?](#how-do-i-use-this).

The examples and tests use hard-coded paths to simplify cross-platform asset handling.

## Ubuntu
Ubuntu installation instructions for examples and tests:

| Action                             | command |
| ---------------------------------- | ------- |
| 1. Navigate to the env directory   |   `cd env/ubuntu_22.04` |
| 2. Install platform dependencies   |   `sudo ./install_deps.sh` |
| 3. Source environment variables    |   `source ./set_env.sh` |
| 4. Install vcpkg libraries         |   `./vcpkg_install.sh` |
| 5. Navigate to the build directory |   `cd ../../build/` |
| 6. Run cmake (and wait for vcpkg)  |   `cmake -DCMAKE_BUILD_TYPE=Release ..` |
| 7. Run make                        |   `make -j8` |
| 8. Navigate to the bin directory   |   `cd ../bin` |
| 9. Run a test                      |   `./interactive` |
