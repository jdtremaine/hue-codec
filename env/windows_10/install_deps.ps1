# Clone vcpkg git repo
cd ../../build
git clone https://github.com/Microsoft/vcpkg.git

# Bootstrap vcpkg
cd vcpkg
.\bootstrap-vcpkg.bat
$env:path += ";" + $pwd.PATH

# Update vcpkg if it's already installed
git pull

# Install vcpkg libraries
cd ../
vcpkg install --triplet x64-windows --x-install-root="vcpkg_installed"

# Make libraries discoverable by intellisense
vcpkg integrate install

# Return to this directory
cd ../env/windows_10
