# Install dependencies for Ubuntu 22.04
# This should be run before vcpkg install

# Platform build environment
sudo apt -y install build-essential cmake git

# OpenCV prerequisites
sudo apt -y install bison

# Realsense / libusb:
sudo apt -y install dh-autoreconf libudev-dev

if [ ! -d "/opt/vcpkg/" ]; then
	echo "Installing vcpkg..."
	git clone https://github.com/Microsoft/vcpkg.git
	sudo mv ./vcpkg /opt/
	/opt/vcpkg/bootstrap-vcpkg.sh
fi
