

#include <Ice/Ice.h>

void
clientCertificateSelectionCallbackExample()
{
    //! [Setting client certificate selection callback]
    CFArrayRef clientCertificateChain = {};
    // Load the client certificate chain from the keychain using SecureTransport APIs.
    auto initData = Ice::InitializationData{
        .clientAuthenticationOptions = Ice::SSL::ClientAuthenticationOptions{
            .clientCertificateSelectionCallback = [clientCertificateChain](const std::string&)
            {
                // Retain the client certificate chain to ensure it remains valid
                // for the duration of the connection. The SSL transport will
                // release it after closing the connection.
                CFRetain(clientCertificateChain);
                return clientCertificateChain;
            }}};
    auto communicator = Ice::initialize(initData);
    // ...
    CFRelease(clientCertificateChain); // Release the CFArrayRef when no longer needed
    //! [Setting client certificate selection callback]
}

void
clientSetTrustedRootCertificatesExample()
{
    //! [Setting client trusted root certificates]
    CFArrayRef rootCerts = {};
    // Populate root certs with X.509 trusted root certificates
    auto initData = Ice::InitializationData{
        .clientAuthenticationOptions = Ice::SSL::ClientAuthenticationOptions{.trustedRootCertificates = rootCerts}};
    auto communicator = Ice::initialize(initData);
    CFRelease(rootCerts); // It is safe to release the rootCerts now.
    //! [Setting client trusted root certificates]
}

void
clientSetNewSessionCallbackExample()
{
    //! [Setting client new session callback]
    auto initData = Ice::InitializationData{
        .clientAuthenticationOptions = Ice::SSL::ClientAuthenticationOptions{
            .sslNewSessionCallback = [](SSLContextRef context, const std::string& host)
            {
                OSStatus status = SSLSetProtocolVersionMin(context, kTLSProtocol13);
                if (status != noErr)
                {
                    // Handle error
                }
            }}};
    //! [Setting client new session callback]
}

void
serverCertificateValidationCallbackExample()
{
    //! [Setting server certificate validation callback]
    auto initData = Ice::InitializationData{
        .clientAuthenticationOptions = Ice::SSL::ClientAuthenticationOptions{
            .serverCertificateValidationCallback = [](SecTrustRef trust, const Ice::SSL::ConnectionInfoPtr& info)
            { return SecTrustEvaluateWithError(trust, nullptr); }}};
    //! [Setting server certificate validation callback]
}