#!/bin/bash
set -eu

# Ensure the repository url is set
if [ -z "${NEXUS_URL}" ]; then
    echo "Error: NEXUS_URL must be set as environment variable."
    exit 1
fi

# Iterate over all .deb and .ddeb files in the current directory and subdirectories
find . -type f \( -name "*.deb" -o -name "*.ddeb" \) | while read -r file; do
    echo "Uploading $file to Nexus..."
    curl -u "${NEXUS_USER}:${NEXUS_PASSWORD}" \
         -H "Content-Type: multipart/form-data" \
         --data-binary "@$file" \
         "${NEXUS_URL}"
    echo "Successfully uploaded: $file"
done
