#!/bin/bash
set -euo pipefail

STAGING="${1:?Usage: $0 <staging-dir> <repo-dir>}"
REPODIR="${2:?Usage: $0 <staging-dir> <repo-dir>}"

# Include noarch in the list of architectures
ARCHES=(i686 x86_64 aarch64 noarch)

# Ensure rpm signing environment is set up
export GPG_TTY=$(tty)

echo "Syncing RPMs from '$STAGING' to '$REPODIR'..."

for arch in "${ARCHES[@]}"; do
  echo "Processing architecture: $arch"

  # Find all RPMs for the current arch
  mapfile -t rpms < <(find "$STAGING" -type f -name "*.${arch}.rpm")

  # Ensure target directory exists
  mkdir -p "$REPODIR/$arch"

  for rpm in "${rpms[@]}"; do
    target="$REPODIR/$arch/$(basename "$rpm")"

    if [[ -f "$target" ]]; then
      echo "Skipping existing: $(basename "$rpm")"
      continue
    fi

    echo "Copying: $(basename "$rpm")"
    cp "$rpm" "$target"

    echo "Signing: $(basename "$rpm")"
    rpmsign --addsign "$target"
  done
done

# Create or update repo metadata
echo "Running createrepo_c in $REPODIR..."
createrepo_c --update "$REPODIR"

# Sign the repomd.xml metadata
REPO_MD="$REPODIR/repodata/repomd.xml"
if [[ -f "$REPO_MD" ]]; then
  echo "Signing repomd.xml..."
  gpg --detach-sign --armor --yes --batch "$REPO_MD"
else
  echo "Warning: repomd.xml not found!"
  exit 1
fi

echo "Repository sync and signing complete."
