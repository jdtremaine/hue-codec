FROM nvidia/cuda:12.0.1-devel-ubuntu22.04 as stage1
SHELL ["/bin/bash", "-c"]

# This dockerfile is intended for benchmarking so it deletes build and cache files.

# To build a docker image that can be used to build binaries, 
# remove the rm commands at the end of each stage.

# Get the repo
# Select the appropriate vcpkg_alternative to build
RUN apt-get update && apt-get -y install git ;\
cd /root ;\
git clone https://github.com/jdtremaine/hue-codec.git ;\
cp /root/hue-codec/env/vcpkg_alternatives/vcpkg_vidcodecs.json /root/hue-codec/vcpkg.json ;\
rm -rf /var/lib/apt/lists/*

from stage1 as stage2

# Install the dependencies
RUN apt-get update && apt-get -y install \
wget \
apt-utils \
ssh \
sudo \
curl zip unzip tar \
pkg-config ;\
/root/hue-codec/env/ubuntu_22.04/install_deps.sh ;\
rm -rf /var/lib/apt/lists/*

from stage2 as stage3

# Add vcpkg to the path
ENV PATH="${PATH}:/opt/vcpkg"

# run vcpkg installer and make the binaries
RUN source /root/hue-codec/env/ubuntu_22.04/set_env_vars.sh ;\
/root/hue-codec/env/ubuntu_22.04/vcpkg_install.sh ;\
cd /root/hue-codec/build ;\
cmake .. ;\
make -j8 ;\
cd .. ;\
rm -rf /root/hue-codec/build ;\
rm -rf /opt/vcpkg
