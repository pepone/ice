

#include <Ice/Ice.h>

#if defined(ICE_USE_SECURE_TRANSPORT)
void
serverCertificateSelectionCallbackExample()
{
    Ice::CommunicatorHolder communicator = Ice::initialize();
    //! [Setting server certificate selection callback]
    CFArrayRef serverCertificateChain = {};
    // Load the server certificate chain from the keychain using SecureTransport APIs.
    communicator->createObjectAdapterWithEndpoints(
        "Hello",
        "ssl -h 127.0.0.1 -p 10000",
        Ice::SSL::ServerAuthenticationOptions{
            .serverCertificateSelectionCallback = [serverCertificateChain](const std::string&)
            {
                // Retain the server certificate chain to ensure it remains valid
                // for the duration of the connection. The SSL transport will
                // release it after closing the connection.
                CFRetain(serverCertificateChain);
                return serverCertificateChain;
            }});
    communicator->waitForShutdown();
    // Release the CFArrayRef when no longer needed
    CFRelease(serverCertificateChain);
    //! [Setting server certificate selection callback]
}

void
serverSetTrustedRootCertificatesExample()
{
    Ice::CommunicatorHolder communicator = Ice::initialize();
    //! [Setting server trusted root certificates]
    CFArrayRef rootCerts = {};
    // Populate root certs with X.509 trusted root certificates
    communicator->createObjectAdapterWithEndpoints(
        "Hello",
        "ssl -h 127.0.0.1 -p 10000",
        Ice::SSL::ServerAuthenticationOptions{.trustedRootCertificates = rootCerts});
    CFRelease(rootCerts); // It is safe to release the rootCerts now.
    //! [Setting server trusted root certificates]
}

void
serverSetNewSessionCallbackExample()
{
    Ice::CommunicatorHolder communicator = Ice::initialize();
    //! [Setting server new session callback]
    communicator->createObjectAdapterWithEndpoints(
        "Hello",
        "ssl -h 127.0.0.1 -p 10000",
        Ice::SSL::ServerAuthenticationOptions{
            .sslNewSessionCallback = [](SSLContextRef context, const std::string& host)
            {
                OSStatus status = SSLSetProtocolVersionMin(context, kTLSProtocol13);
                if (status != noErr)
                {
                    // Handle error
                }
            }});
    //! [Setting server new session callback]
}

void
clientCertificateValidationCallbackExample()
{
    Ice::CommunicatorHolder communicator = Ice::initialize();
    //! [Setting client certificate validation callback]
    communicator->createObjectAdapterWithEndpoints(
        "Hello",
        "ssl -h 127.0.0.1 -p 10000",
        Ice::SSL::ServerAuthenticationOptions{
            .clientCertificateRequired = kAlwaysAuthenticate,
            .clientCertificateValidationCallback = [](SecTrustRef trust, const Ice::SSL::ConnectionInfoPtr& info)
            { return SecTrustEvaluateWithError(trust, nullptr); }});
    //! [Setting client certificate validation callback]
}
#elif defined(ICE_USE_OPENSSL)
void
serverCertificateSelectionCallbackExample()
{
    Ice::CommunicatorHolder communicator = Ice::initialize();
    //! [Setting server certificate selection callback]
    SSL_CTX* sslContext = SSL_CTX_new(TLS_method());
    // Load the server certificate chain from the keychain using SecureTransport APIs.
    communicator->createObjectAdapterWithEndpoints(
        "Hello",
        "ssl -h 127.0.0.1 -p 10000",
        Ice::SSL::ServerAuthenticationOptions{
            .serverSSLContextSelectionCallback = [sslContext](const std::string&)
            {
                // Ensure the SSL context remains valid for the lifetime of the connection.
                SSL_CTX_up_ref(sslContext);
                return sslContext;
            }});
    communicator->waitForShutdown();
    // Release the SSLContext when no longer needed
    SSL_CTX_free(sslContext);
    //! [Setting server certificate selection callback]
}

void
serverSetNewSessionCallbackExample()
{
    Ice::CommunicatorHolder communicator = Ice::initialize();
    //! [Setting server new session callback]
    communicator->createObjectAdapterWithEndpoints(
        "Hello",
        "ssl -h 127.0.0.1 -p 10000",
        Ice::SSL::ServerAuthenticationOptions{
            .sslNewSessionCallback = [](SSL* ssl, const std::string& host)
            {
                if (!SSL_set_tlsext_host_name(ssl, host.c_str()))
                {
                    // Handle error
                }
            }});
    //! [Setting server new session callback]
}

void
clientCertificateValidationCallbackExample()
{
    Ice::CommunicatorHolder communicator = Ice::initialize();
    //! [Setting client certificate validation callback]
    communicator->createObjectAdapterWithEndpoints(
        "Hello",
        "ssl -h 127.0.0.1 -p 10000",
        Ice::SSL::ServerAuthenticationOptions{
            .clientCertificateValidationCallback =
                [](bool verified, X509_STORE_CTX* ctx, const Ice::SSL::ConnectionInfoPtr& info) { return verified; }});
    //! [Setting client certificate validation callback]
}
#elif defined(ICE_USE_SCHANNEL)
#endif
