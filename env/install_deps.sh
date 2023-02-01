# Install dependencies for Ubuntu 22.04
# This should be run before vcpkg install

# Platform build environment
sudo apt-get -y install build-essential cmake git

# OpenCV prerequisites
sudo apt-get -y install bison

# Realsense / libusb:
sudo apt-get -y install dh-autoreconf libudev-dev

# OpenCV Suport for camera and video formats
sudo apt-get -y install libavcodec-dev libavformat-dev libswscale-dev

# OpenCV support for image formats
sudo apt-get -y install libpng-dev libjpeg-dev libwebp-dev

# Video codecs
sudo apt-get -y install x264 x265

# For libvpx
sudo apt-get -y install nasm

# For glib
sudo apt-get -y install python3-distutils

# For libepoxy 
sudo apt-get -y install libx11-dev libgles2-mesa-dev

# For cairo / ("install Xorg dependencies to use feature x11")
sudo apt-get -y install libx11-dev libxft-dev libxext-dev

# For at-spi2-core
sudo apt-get -y install libdbus-1-dev libxi-dev libxtst-dev

# for at-spi2-atk
sudo apt-get install -y libdbus-1-dev

# For gtk3
sudo apt-get -y install libxrandr-dev

# vcpkg dependencies
sudo apt-get -y install curl zip unzip tar

# Install/update vcpkg
if [ ! -d "/opt/vcpkg/" ]; then
	echo "Installing vcpkg..."
	git clone https://github.com/Microsoft/vcpkg.git
	sudo mv ./vcpkg /opt/
	/opt/vcpkg/bootstrap-vcpkg.sh
else
	echo "Updating vcpkg..."
	cd /opt/vcpkg/
	git pull
	/opt/vcpkg/vcpkg update
fi
