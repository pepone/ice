FROM registry.access.redhat.com/ubi9

# Install required build tools and dependencies
RUN dnf install -y rpmdevtools && dnf clean all
