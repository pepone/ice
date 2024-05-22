//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#include <Ice/Ice.h>

void
serverCertificateSelectionCallbackExample()
{
    Ice::CommunicatorHolder communicator = Ice::initialize();
    //! [serverSSLContextSelectionCallback]
    SSL_CTX* sslContext = SSL_CTX_new(TLS_method());
    // Load the server certificate chain from the keychain using SecureTransport
    // APIs.
    communicator->createObjectAdapterWithEndpoints(
        "Hello",
        "ssl -h 127.0.0.1 -p 10000",
        Ice::SSL::ServerAuthenticationOptions{
            .serverSSLContextSelectionCallback =
                [sslContext](const std::string&)
            {
                // Ensure the SSL context remains valid for the lifetime of the
                // connection.
                SSL_CTX_up_ref(sslContext);
                return sslContext;
            }});
    communicator->waitForShutdown();
    // Release the SSLContext when no longer needed
    SSL_CTX_free(sslContext);
    //! [serverSSLContextSelectionCallback]
}

void
serverSetNewSessionCallbackExample()
{
    Ice::CommunicatorHolder communicator = Ice::initialize();
    //! [sslNewSessionCallback]
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
    //! [sslNewSessionCallback]
}

void
clientCertificateValidationCallbackExample()
{
    Ice::CommunicatorHolder communicator = Ice::initialize();
    //! [clientCertificateValidationCallback]
    communicator->createObjectAdapterWithEndpoints(
        "Hello",
        "ssl -h 127.0.0.1 -p 10000",
        Ice::SSL::ServerAuthenticationOptions{
            .clientCertificateValidationCallback =
                [](bool verified,
                   X509_STORE_CTX* ctx,
                   const Ice::SSL::ConnectionInfoPtr& info)
            { return verified; }});
    //! [clientCertificateValidationCallback]
}
