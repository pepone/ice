FROM registry.access.redhat.com/ubi10

# Install required build tools and dependencies
RUN --mount=type=secret,id=rh_credentials \
    source /run/secrets/rh_credentials && \
    subscription-manager register --username "$RH_USERNAME" --password "$RH_PASSWORD" && \
    dnf install -y rpmdevtools && \
    dnf clean all && \
    subscription-manager unregister && \
    rm -rf /etc/pki/entitlement /etc/rhsm /etc/yum.repos.d/redhat.repo
