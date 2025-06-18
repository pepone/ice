FROM registry.access.redhat.com/ubi10

RUN subscription-manager register --username $RH_USERNAME --password $RH_PASSWORD

# Delete /etc/rhsm-host to use entitlements from the build container
RUN dnf install -y \
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

# Install build dependencies.
RUN dnf builddep /usr/src/ice.spec && \
    dnf clean all \
    subscription-manager unregister

RUN dnf clean all && \
    subscription-manager unregister && \
    rm -rf /etc/pki/entitlement /etc/rhsm /etc/yum.repos.d/redhat.repo
