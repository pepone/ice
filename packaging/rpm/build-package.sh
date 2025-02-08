#!/bin/bash
set -eux  # Exit on error, print commands

# Define build root directory
RPM_BUILD_ROOT="/workspace/build"

# Ensure necessary directories exist
mkdir -p "$RPM_BUILD_ROOT"/{BUILD,BUILDROOT,RPMS,SOURCES,SPECS,SRPMS}

# Copy Ice spec file
ICE_SPEC_SRC="/workspace/ice/packaging/rpm/ice.spec"
ICE_SPEC_DEST="$RPM_BUILD_ROOT/SPECS/ice.spec"

cp "$ICE_SPEC_SRC" "$ICE_SPEC_DEST"

# Validate TARGET_ARCH
VALID_ARCHS=("x86_64" "i686" "aarch64")
if [[ -z "${TARGET_ARCH:-}" || ! " ${VALID_ARCHS[@]} " =~ " ${TARGET_ARCH} " ]]; then
    echo "Error: TARGET_ARCH is not set or invalid. Use one of: ${VALID_ARCHS[*]}"
    exit 1
fi

# Define common RPM macros
RPM_MACROS=(-D "_topdir $RPM_BUILD_ROOT"  -D "vendor ZeroC, Inc.")

# Download sources
cd "$RPM_BUILD_ROOT/SOURCES"
spectool -g "$ICE_SPEC_DEST" || { echo "Error: Failed to download sources."; exit 1; }

# Build source RPM
rpmbuild -bs "$ICE_SPEC_DEST" "${RPM_MACROS[@]}" --target="$TARGET_ARCH"

# Build binary RPM
rpmbuild -bb "$ICE_SPEC_DEST" "${RPM_MACROS[@]}" --target="$TARGET_ARCH"
