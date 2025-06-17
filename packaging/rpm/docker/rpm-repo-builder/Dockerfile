# Image: ice-rpm-repo-builder
# Purpose: Build and sign ZeroC Ice RPM packages and create YUM repositories

FROM rockylinux:9

# Install tools required for RPM signing and repository creation
RUN dnf install -y \
    rpmdevtools \
    curl-minimal \
    createrepo_c \
    gnupg2 \
    rpm-sign && \
    dnf clean all
