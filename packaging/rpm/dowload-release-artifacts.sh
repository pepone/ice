#!/bin/bash
set -euxo pipefail

# Initialize variables
distro=""
tag=""
run_id=""
mcpp_version=v2.7.2.19

# Simple flag parser
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
  rhel9)
    SUPPORTED_ARCHS=("i686" "x86_64" "aarch64")
    ;;
  rhel10 | amzn2023)
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

# Ensure --tag and --run-id are not both set
if [[ -n "$tag" && -n "$run_id" ]]; then
  echo "Error: --tag and --run-id are mutually exclusive"
  exit 1
fi

# Create and switch to staging directory
STAGING="staging-$distro"
mkdir -p "$STAGING"
cd "$STAGING"

# Download from Ice release tag
if [[ -n "$tag" ]]; then

  # When building from a tag download MCPP RPMs for rhel9 or rhel10
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

  for arch in "${SUPPORTED_ARCHS[@]}"; do
    target_dir="ice/$tag/$arch"
    mkdir -p "$target_dir"
    pushd "$target_dir" > /dev/null
    gh release download "$mcpp_version" --repo zeroc-ice/mcpp -p "rpm-packages-$distro-$arch*"
    unzip -x rpm-packages-$distro-$arch.zip
    rm -f rpm-packages-$distro-$arch.zip
    popd > /dev/null
  done
  popd > /dev/null
fi

# Download from GitHub Actions run
if [[ -n "$run_id" ]]; then
  for arch in "${SUPPORTED_ARCHS[@]}"; do
    target_dir="ice/$run_id/$arch"
    mkdir -p "$target_dir"
    pushd "$target_dir" > /dev/null
    gh run download "$run_id" --repo zeroc-ice/ice -p "rpm-packages-$distro-$arch*"
    popd > /dev/null
  done
fi

