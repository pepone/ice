FROM registry.access.redhat.com/ubi10

# See: Docker builds using Subscription Manager
# https://docs.redhat.com/en/documentation/openshift_container_platform/4.5/html/builds/running-entitled-builds

# Copy subscription manager configurations and entitlements from a registered system.
COPY rhel10/entitlement /etc/pki/entitlement
COPY rhel10/rhsm.conf /etc/rhsm
COPY rhel10/ca /etc/rhsm/ca

# Delete /etc/rhsm-host to use entitlements from the build container
RUN rm /etc/rhsm-host && \
    # Install required build tools and dependencies
    subscription-manager repos --enable codeready-builder-for-rhel-10-$(uname -m)-rpms && \
    dnf install -y \
    rpmdevtools \
    make \
    curl-minimal \
    git \
    gcc-c++ \
    glibc-devel \
    libstdc++-devel \
    redhat-rpm-config \
    glibc-static \
    libstdc++-static && \
    dnf clean all

# Copy MCPP spec file into the container
COPY ice/packaging/rpm/ice.spec /usr/src/ice.spec

# Install build dependencies. Do not use dnf builddep as it doesn't obey multilib_policy=all.
RUN dnf builddep /usr/src/ice.spec && \
    dnf clean all && \
    # Remove entitlements and Subscription Manager configs
    rm -rf /etc/pki/entitlement && \
    rm -rf /etc/rhsm/ca

LABEL org.opencontainers.image.source=https://github.com/zeroc-ice/ice
LABEL org.opencontainers.image.description="Ice Red Hat Enterprise Linux 10 Docker image for building RPM packages"
