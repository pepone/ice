#!/bin/bash
set -eux -o pipefail

# Build slice2swift zip for SwiftPM
#
# This script builds slice2swift and creates a zip file for distribution.
# The zip is placed at cpp/bin/slice2swift-<version>.zip.
#
# Usage:
#   ./packaging/swift/build-slice2swift.sh
#
# Environment:
#   ICE_VERSION    Override the version (default: from config/version.env)

# Get Git repository root directory
root_dir=$(git rev-parse --show-toplevel)
cd "$root_dir"

# Load version from config/version.env if not set
if [[ -z "${ICE_VERSION:-}" ]]; then
    source config/version.env
    ICE_VERSION="${VERSION}"
fi

echo "Building slice2swift zip version ${ICE_VERSION}"

# Build slice2swift using make
make -C cpp bin/slice2swift.zip OPTIMIZE=yes

zip_path="cpp/bin/slice2swift.zip"
if [[ ! -f "$zip_path" ]]; then
    echo "Error: zip file not found at $zip_path"
    exit 1
fi

# Rename to versioned zip for distribution
versioned_zip="cpp/bin/slice2swift-${ICE_VERSION}.zip"
mv "$zip_path" "$versioned_zip"
echo "Created zip at: $versioned_zip"

# Print checksum for convenience
checksum=$(shasum -a 256 "$versioned_zip" | cut -d ' ' -f 1)
echo "SHA256 checksum: $checksum"

echo "Done."
