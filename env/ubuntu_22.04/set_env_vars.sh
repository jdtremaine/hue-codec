SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
BIN_DIR=$(realpath $SCRIPT_DIR/../../bin)
mkdir -p $BIN_DIR
BUILD_DIR=$(realpath $SCRIPT_DIR/../../build)
mkdir -p $BUILD_DIR

VCPKG_DIR="/opt/vcpkg"

# Set Environment variables
if [[ "$0" = "$BASH_SOURCE" ]]; then
	echo ""
	echo "Warning: This script is designed to be sourced, not executed."
	echo "To source the environment variables into the current shell, run:" 
	echo "source ./set_env.sh"
else
	# Disable OpenCV logging
	echo "Disabling OpenCV logging..."
	export OPENCV_LOG_LEVEL=OFF # Disable OpenCV logging

	echo "Setting vcpkg global directory to $VCPKG_DIR..."
	export VCPKG_ROOT=$VCPKG_DIR

	echo "Add vcpkg to path"
	export PATH="/opt/vcpkg:$PATH"

	echo "Setting vcpkg toolchain environment variable for CMake..."
	export CMAKE_TOOLCHAIN_FILE=$VCPKG_DIR/scripts/buildsystems/vcpkg.cmake

	echo "Setting CMake executable output directory to $BIN_DIR..."
	export CMAKE_RUNTIME_OUTPUT_DIRECTORY=$BIN_DIR

	echo "Environment variables set."
fi
