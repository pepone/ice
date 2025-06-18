#!/bin/bash
set -euxo pipefail

# Helper script to download Ice RPM artifacts from a GitHub release or a GitHub Actions run.
#
# To download the RPMs for a tagged release, use:
#
#   ./download-release-artifacts.sh --distro rhel9 --tag v3.8.0 --staging-dir ~/staging
#
# This downloads the Ice RPM artifacts for the specified distribution from the GitHub release page.
# For `rhel9` and `rhel10`, it also includes the MCPP RPMs from the `zeroc-ice/mcpp` repository,
# which are distributed as part of the ZeroC RPM repository for those platforms.
#
# To download RPMs from a nightly CI build, use `--run-id` with the GitHub Actions run ID:
#
#   ./download-release-artifacts.sh --distro rhel9 --run-id 15696647397 --staging-dir ~/staging
#
# This downloads the Ice RPM artifacts for the specified distribution from the GitHub Actions run
# corresponding to the given run ID.
#
# Arguments:
#
#   --distro       A supported RPM distribution to download artifacts for: rhel9 | rhel10 | amzn2023
#   --tag          A release tag from the zeroc-ice/ice repository to download assets from.
#   --run-id       A GitHub Actions run ID for a nightly CI build to download assets from.
#   --staging-dir  The base directory in which to store the downloaded artifacts. Files will be placed under 
#                  $staging_dir/$distro.
#
# Note: `--tag` and `--run-id` are mutually exclusive; only one may be specified.


# Initialize arguments
distro=""
tag=""
run_id=""
mcpp_version=v2.7.2.19
staging_dir=""

# Parse arguments
while [[ $# -gt 0 ]]; do
  case "$1" in
    --distro)
      distro="$2"
      shift 2
      ;;
    --tag)
      tag="$2"
      shift 2
      ;;
    --run-id)
      run_id="$2"
      shift 2
      ;;
    --staging-dir)
      staging_dir="$2"
      shift 2
      ;;
    -*)
      echo "Unknown option: $1"
      exit 1
      ;;
    *)
      echo "Unexpected argument: $1"
      exit 1
      ;;
  esac
done

# Validate distro
case "$distro" in
  rhel9 | rhel10 | amzn2023)
    SUPPORTED_ARCHS=("x86_64" "aarch64")
    ;;
  "")
    echo "Error: --distro is required (rhel9, rhel10, amzn2023)"
    exit 1
    ;;
  *)
    echo "Unsupported distro: $distro"
    exit 1
    ;;
esac

# Ensure that mutually exclusive options are not both set.
if [[ -n "$tag" && -n "$run_id" ]]; then
  echo "Error: --tag and --run-id are mutually exclusive"
  exit 1
fi

# Ensure --staging-dir is valid.
if [[ -z "$staging_dir" || ! -d "$staging_dir" ]]; then
  echo "Error: --staging-dir must be set and point to an existing directory"
  exit 1
fi

# Set up per-distro staging subdirectory.
STAGING="$staging_dir/$distro"
mkdir -p "$STAGING"
cd "$STAGING"

# Download from GitHub release tag.
if [[ -n "$tag" ]]; then

  # Download MCPP RPMs for rhel9 and rhel10.
  if [[ "$distro" == "rhel9" || "$distro" == "rhel10" ]]; then
    for arch in "${SUPPORTED_ARCHS[@]}"; do
      target_dir="mcpp/$mcpp_version/$arch"
      mkdir -p "$target_dir"
      pushd "$target_dir" > /dev/null
      gh release download "$mcpp_version" --repo zeroc-ice/mcpp -p "rpm-packages-$distro-$arch.zip"
      unzip -x rpm-packages-$distro-$arch.zip
      rm -f rpm-packages-$distro-$arch.zip
      popd > /dev/null
    done
  fi

  # Download Ice RPMs for each arch from the release tag.
  for arch in "${SUPPORTED_ARCHS[@]}"; do
    target_dir="ice/$tag/$arch"
    mkdir -p "$target_dir"
    pushd "$target_dir" > /dev/null
    gh release download "$tag" --repo zeroc-ice/ice -p "rpm-packages-$distro-$arch*"
    unzip -x rpm-packages-$distro-$arch.zip
    rm -f rpm-packages-$distro-$arch.zip
    popd > /dev/null
  done
fi

# Download from GitHub Actions run.
if [[ -n "$run_id" ]]; then
  for arch in "${SUPPORTED_ARCHS[@]}"; do
    target_dir="ice/$run_id/$arch"
    mkdir -p "$target_dir"
    pushd "$target_dir" > /dev/null
    gh run download "$run_id" --repo zeroc-ice/ice -p "rpm-packages-$distro-$arch*"
    popd > /dev/null
  done
fi
