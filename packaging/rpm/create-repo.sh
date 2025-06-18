#!/bin/bash
set -euo pipefail

# Ensure GPG_KEY and GPG_KEY_ID are set
: "${GPG_KEY:?GPG_KEY environment variable is not set}"
: "${GPG_KEY_ID:?GPG_KEY_ID environment variable is not set}"

# Import the GPG key
echo "$GPG_KEY" | gpg --batch --import

# Check that the key was successfully imported
if ! gpg --list-secret-keys "$GPG_KEY_ID" > /dev/null 2>&1; then
  echo "Error: GPG key ID $GPG_KEY_ID was not imported successfully."
  exit 1
fi

# Set up ~/.rpmmacros for rpmsign
cat > ~/.rpmmacros <<EOF
%_signature gpg
%_gpg_name $GPG_KEY_ID
%_gpg_path ~/.gnupg
%__gpg_check_password_cmd /bin/true
%__gpg /usr/bin/gpg
EOF


STAGING="${1:?Usage: $0 <staging-dir> <repo-dir>}"
REPODIR="${2:?Usage: $0 <staging-dir> <repo-dir>}"

# Include noarch in the list of architectures
ARCHES=(x86_64 aarch64 noarch)

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
