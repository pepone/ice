#!/bin/bash
set -eux  # Exit on error, print commands

# Create build directory and copy Debian packaging files
mkdir -p /workspace/build
cp -rfv /workspace/ice/packaging/dpkg/debian /workspace/build

# Determine ICE_VERSION
if [[ -n "${ICE_VERSION:-}" ]]; then
    echo "Updating UNRELEASED changelog entry to: ${ICE_VERSION}"
    dch --changelog /workspace/build/debian/changelog --newversion "$ICE_VERSION" \
        --force-bad-version --distribution UNRELEASED \
        "Ice Nightly build $ICE_VERSION"
else
    ICE_VERSION=$(dpkg-parsechangelog --file /workspace/build/debian/changelog --show-field Version)
fi

# Generate a tarball of the current repository state for the given ICE_VERSION
echo "Creating tarball for ICE_VERSION=$ICE_VERSION"
cd /workspace/ice
git archive --format=tar.gz -o /workspace/zeroc-ice_${ICE_VERSION}.orig.tar.gz HEAD

# Unpack the source tarball
cd /workspace/build
tar xzf ../zeroc-ice_${ICE_VERSION}.orig.tar.gz

# Build the source package (-S generates .dsc and .tar.gz files)
dpkg-buildpackage -S

# Build the binary packages (-b creates .deb files, -uc -us skips signing)
dpkg-buildpackage -b -uc -us
