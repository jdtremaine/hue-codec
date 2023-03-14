cmake_minimum_required(VERSION 3.15)

# Note that CMAKE_TOOLCHAIN_PATH should be set with an environment variable if using vcpkg
# Alternatively, it can be set here:
# set(CMAKE_TOOLCHAIN_PATH "/opt/vcpkg/scripts/buildsystems/vcpkg.cmake")

# Use the CMAKE_RUNTIME_OUTPUT_DIRECTORY environment variable
# This sets the binary file output directory
if (DEFINED ENV{CMAKE_RUNTIME_OUTPUT_DIRECTORY})
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY $ENV{CMAKE_RUNTIME_OUTPUT_DIRECTORY})
endif()

set(CMAKE_CXX_FLAGS "-Wall -Wextra -std=c++14")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

project(hue_codec CXX)
include_directories(./include)

find_package(fmt CONFIG REQUIRED)
find_package(doctest CONFIG REQUIRED)
find_package(OpenCV CONFIG REQUIRED)
find_package(realsense2 CONFIG REQUIRED)

# Tests
add_executable(tests test/tests.cpp)
target_link_libraries(tests PRIVATE fmt::fmt)
target_link_libraries(tests PRIVATE doctest::doctest)
target_link_libraries(tests PRIVATE ${OpenCV_LIBS})

# Comparison to RealSense encoder and reference decoder
add_executable(comparison test/comparison.cpp)
target_link_libraries(comparison PRIVATE fmt::fmt)
target_link_libraries(comparison PRIVATE doctest::doctest)
target_link_libraries(comparison PRIVATE opencv_core opencv_imgproc opencv_highgui)
target_link_libraries(comparison PRIVATE realsense2::realsense2)

# Benchmarks
add_executable(benchmarks test/benchmarks.cpp)
target_link_libraries(benchmarks PRIVATE fmt::fmt)
target_link_libraries(benchmarks PRIVATE doctest::doctest)
target_link_libraries(benchmarks PRIVATE opencv_core opencv_imgproc opencv_highgui opencv_videoio)

# Interactive Visualizers
add_executable(interactive test/interactive.cpp)
target_link_libraries(interactive PRIVATE fmt::fmt)
target_link_libraries(interactive PRIVATE doctest::doctest)
target_link_libraries(interactive PRIVATE opencv_core opencv_imgproc opencv_highgui opencv_videoio)
