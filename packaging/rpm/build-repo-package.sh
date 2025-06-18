#!/bin/bash
set -eux  # Exit on error, print commands

# Define build root directory
RPM_BUILD_ROOT="/workspace/build"

# Ensure necessary directories exist
mkdir -p "$RPM_BUILD_ROOT"/{BUILD,BUILDROOT,RPMS,SOURCES,SPECS,SRPMS}

# Copy ice-repo spec file
SPEC_SRC="/workspace/ice/packaging/rpm/ice-repo.spec"
SPEC_DEST="$RPM_BUILD_ROOT/SPECS/ice-repo.spec"

cp "$SPEC_SRC" "$SPEC_DEST"

# Define common RPM macros
RPM_MACROS=()
RPM_MACROS+=(--define "_topdir $RPM_BUILD_ROOT")
RPM_MACROS+=(--define "vendor ZeroC, Inc.")

# Download sources
cp "/workspace/ice/packaging/rpm/$DISTRIBUTION/zeroc-ice3.8.repo" "$RPM_BUILD_ROOT/SOURCES"

# Build source RPM
rpmbuild -bs "$SPEC_DEST" "${RPM_MACROS[@]}"

# Build binary RPM
rpmbuild -bb "$SPEC_DEST" "${RPM_MACROS[@]}"
