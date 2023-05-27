cmake_minimum_required(VERSION 3.15)

# Note that CMAKE_TOOLCHAIN_PATH should be set with an environment variable if using vcpkg
# Alternatively, it can be set here:
# set(CMAKE_TOOLCHAIN_PATH "/opt/vcpkg/scripts/buildsystems/vcpkg.cmake")

# Use the CMAKE_RUNTIME_OUTPUT_DIRECTORY environment variable
# This sets the binary file output directory
if (DEFINED ENV{CMAKE_RUNTIME_OUTPUT_DIRECTORY})
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY $ENV{CMAKE_RUNTIME_OUTPUT_DIRECTORY})
endif()

project(hue-codec CXX)

# Linux-specific compiler flags
if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
	message(STATUS "Linux flags")

	# Linux compiler flags
	set(CMAKE_CXX_FLAGS "-Wall -Wextra -std=c++14")
	set(CMAKE_CXX_FLAGS_DEBUG "-g")
	set(CMAKE_CXX_FLAGS_RELEASE "-O3")

	# Build compile_commands.json for clang tools
	set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
endif()

include_directories(./include)

find_package(fmt CONFIG REQUIRED)
find_package(doctest CONFIG REQUIRED)
find_package(OpenCV CONFIG REQUIRED)
find_package(realsense2 CONFIG)

# Tests
add_executable(tests test/tests.cpp)
target_link_libraries(tests PRIVATE fmt::fmt)
target_link_libraries(tests PRIVATE doctest::doctest)
target_link_libraries(tests PRIVATE ${OpenCV_LIBS})

# Interactive Visualizers
add_executable(interactive test/interactive.cpp)
target_link_libraries(interactive PRIVATE fmt::fmt)
target_link_libraries(interactive PRIVATE doctest::doctest)
target_link_libraries(interactive PRIVATE opencv_core opencv_imgproc opencv_highgui opencv_videoio)

# Example - depth frame
add_executable(example_frame src/example_frame.cpp)
target_link_libraries(example_frame PRIVATE opencv_core opencv_imgproc opencv_highgui opencv_videoio)

# Example - depth sequence
add_executable(example_sequence src/example_sequence.cpp)
target_link_libraries(example_sequence PRIVATE opencv_core opencv_imgproc opencv_highgui opencv_videoio)

# Benchmarks (to run all benchmarks ffmpeg, codecs, and cuda must be installed )
add_executable(benchmarks test/benchmarks.cpp)
target_link_libraries(benchmarks PRIVATE fmt::fmt)
target_link_libraries(benchmarks PRIVATE doctest::doctest)
target_link_libraries(benchmarks PRIVATE ${OpenCV_LIBS})

if(RealSense2_FOUND OR realsense2_FOUND)
	# Example - depth sensor (requires realsense2 library)
	add_executable(example_sensor src/example_sensor.cpp)
	target_link_libraries(example_sensor PRIVATE ${OpenCV_LIBS})
	target_link_libraries(example_sensor PRIVATE realsense2::realsense2)

	# Comparison to RealSense encoder and reference decoder (requires realsense2 library)
	add_executable(comparison test/comparison.cpp)
	target_link_libraries(comparison PRIVATE fmt::fmt)
	target_link_libraries(comparison PRIVATE doctest::doctest)
	target_link_libraries(comparison PRIVATE ${OpenCV_LIBS})
	target_link_libraries(comparison PRIVATE realsense2::realsense2)
endif()
