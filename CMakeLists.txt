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

# Tests
add_executable(tests test/tests.cpp)
target_link_libraries(tests PRIVATE fmt::fmt)
target_link_libraries(tests PRIVATE doctest::doctest)
target_link_libraries(tests PRIVATE ${OpenCV_LIBS})

