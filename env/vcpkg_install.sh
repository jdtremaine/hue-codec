SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
cd $SCRIPT_DIR/../..
mkdir -p build
cd build
vcpkg install --x-install-root="vcpkg_installed"
