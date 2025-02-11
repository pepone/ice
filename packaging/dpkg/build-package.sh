#!/bin/bash
set -eux  # Exit on error, print commands

# Ensure ICE_VERSION is set
if [ -z "$ICE_VERSION" ]; then
    echo "Error: ICE_VERSION is not set!"
    exit 1
fi

# Generate the upstream tarball
echo "Creating tarball for ICE_VERSION=$ICE_VERSION"
cd /workspace/ice
git archive --format=tar.gz -o /workspace/zeroc-ice_${ICE_VERSION}.orig.tar.gz HEAD

# Create build directory and unpack
mkdir -p /workspace/build
cd /workspace/build
tar xzf ../zeroc-ice_${ICE_VERSION}.orig.tar.gz

# Copy Debian packaging files into the build directory
cp -rfv /workspace/ice/packaging/dpkg/debian .

# Build the source package
dpkg-buildpackage -S

# Build the binary packages
dpkg-buildpackage -b -uc -us
