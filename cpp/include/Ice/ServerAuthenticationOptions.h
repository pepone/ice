//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#ifndef ICE_SSL_SERVER_AUTHENTICATION_OPTIONS_H
#define ICE_SSL_SERVER_AUTHENTICATION_OPTIONS_H

#include "SSLConfig.h"
#include "SSLConnectionInfo.h"

#include <functional>

namespace Ice::SSL
{
    /**
     * The SSL configuration properties for server connections.
     */
    struct ServerAuthenticationOptions
    {
#if defined(ICE_USE_SCHANNEL)
        /**
         * A callback that allows selecting the server credentials based on the name of the object adapter that
         * accepts the connection.
         *
         * @param adapterName The name of the object adapter that accepted the connection.
         * @return The server credentials. The credentials must remain valid for the duration of the connection.
         *
         * [See Detailed Schannel documentation on Schannel credentials](
         * https://learn.microsoft.com/en-us/windows/win32/secauthn/acquirecredentialshandle--schannel)
         */
        std::function<SCHANNEL_CRED(const std::string& adapterName)> serverCredentialsSelectionCallback;

        /**
         * Whether or not the server requires that the client provides a certificate.
         */
        bool clientCertificateRequired;

        /**
         * A callback that allows manually validating the client certificate chain. When the verification callback
         * returns false, the connection will be aborted with an Ice::SecurityException.
         *
         * @param context A CtxtHandle representing the security context associated with the current connection. This
         * context contains security data relevant for validation, such as the client's certificate chain and cipher
         * suite.
         * @param info The IceSSL::ConnectionInfoPtr object that provides additional connection-related data
         * which might be relevant for contextual certificate validation.
         * @return true if the certificate chain is valid and the connection should proceed; false if the certificate
         * chain is invalid and the connection should be aborted. An exception may be thrown to indicate a custom
         * error during the validation process.
         *
         * [See Manually Validating Schannel Credentials for more
         * details](https://learn.microsoft.com/en-us/windows/win32/secauthn/manually-validating-schannel-credentials).
         */
        std::function<bool(CtxtHandle context, const IceSSL::ConnectionInfoPtr& info)>
            clientCertificateValidationCallback;

#elif defined(ICE_USE_SECURE_TRANSPORT)
        /**
         * A callback that allows selecting the server's SSL certificate chain based on the name of the object adapter
         * that accepts the connection.
         *
         * @remarks This callback is invoked by the SSL transport for each new incoming connection before starting the
         * SSL handshake to determine the appropriate server certificate chain. The callback should return a CFArrayRef
         * that represents the server's certificate chain, or NULL if no certificate chain should be used for the
         * connection. The SSL transport takes ownership of the returned CFArrayRef and releases it when the connection
         * is closed.
         *
         * @param adapterName The name of the object adapter that accepted the connection.
         * @return CFArrayRef containing the server's certificate chain, or NULL to indicate that no certificate is
         * used.
         *
         * Example of setting serverCertificateSelectionCallback:
         * ```cpp
         * _serverCertificateChain = CFArrayCreate(...); // Populate with X.509 certificate/key pair
         * communicator->createObjectAdapterWithEndpoints(
         *   "Hello",
         *   "ssl -h 127.0.0.1 -p 10000",
         *   ServerAuthenticationOptions {
         *     .serverCertificateSelectionCallback =
         *     [this](const std::string&)
         *     {
         *       // Retain the server's certificate chain to ensure it remains valid for the duration
         *       // of the connection. The SSL transport will release it after closing the connection.
         *       CFRetain(_serverCertificateChain);
         *       return _serverCertificateChain;
         *     });
         * ...
         * CFRelease(_serverCertificateChain); // Release the CFArrayRef when no longer needed
         * ```
         *
         * See the Secure Transport documentation for requirements on the certificate chain format:
         * [SSLSetCertificate](https://developer.apple.com/documentation/security/1392400-sslsetcertificate?changes=_3&language=objc)
         */
        std::function<CFArrayRef(const std::string& adapterName)> serverCertificateSelectionCallback;

        /**
         * The trusted root certificates used for validating the client's certificate chain. If this field is set, the
         * client's certificate chain is validated against these certificates; otherwise, the system's default root
         * certificates are used.
         *
         * @remarks The trusted root certificates are used by both the default validation callback, and by custom
         * validation callback set in clientCertificateValidationCallback.
         *
         * This is equivalent to calling
         * [SecTrustSetAnchorCertificates](https://developer.apple.com/documentation/security/1396098-sectrustsetanchorcertificates?language=objc)
         * with the CFArrayRef object; and
         * [SecTrustSetAnchorCertificatesOnly](https://developer.apple.com/documentation/security/1399071-sectrustsetanchorcertificatesonl?language=objc)
         * with the `anchorCertificatesOnly` parameter set to true.
         *
         * The application must ensure that the CFArrayRef remains valid during the setup of the ObjectAdapter. It is
         * also the application's responsibility to release the CFArrayRef object after the ObjectAdapter has been
         * created to prevent memory leaks.
         *
         * Example of setting trustedRootCertificates:
         * ```cpp
         * CFArrayRef rootCerts = CFArrayCreate(...); // Populate with X.509 certificates
         * communicator->createObjectAdapterWithEndpoints(
         *   "Hello",
         *   "ssl -h 127.0.0.1 -p 10000",
         *   ServerAuthenticationOptions{ .trustedRootCertificates = rootCerts });
         * CFRelease(rootCerts);  // It is safe to release the CFArrayRef now
         * ```
         */
        CFArrayRef trustedRootCertificates;

        /**
         * The requirements for client-side authentication
         *
         * [see SSLAuthenticate](https://developer.apple.com/documentation/security/sslauthenticate)
         */
        SSLAuthenticate clientCertificateRequired;

        /**
         * A callback that is invoked before initiating a new SSL handshake. This callback provides an opportunity to
         * customize the SSL parameters for the session based on specific server settings or requirements.
         *
         * Example of setting sslNewSessionCallback:
         * ```cpp
         * communicator->createObjectAdapterWithEndpoints(
         *   "Hello",
         *   "ssl -h 127.0.0.1 -p 10000",
         *   ServerAuthenticationOptions{
         *     .sslNewSessionCallback =
         *       [](SSLContextRef context, const std::string& adapterName)
         *       {
         *         // Customize the ssl context using SecureTransport API.
         *         OSStatus status = SSLSetProtocolVersionMin(context, kTLSProtocol13);
         *         if(status != noErr)
         *         {
         *            // Handle error
         *         }
         *         ...
         *       }
         * });
         * ```
         *
         * @param context An opaque type that represents an SSL session context object.
         * @param adapterName The name of the object adapter that accepted the connection.
         */
        std::function<void(SSLContextRef context, const std::string& adapterName)> sslNewSessionCallback;

        /**
         * A callback that allows manually validating the client certificate chain. When the verification callback
         * returns false, the connection will be aborted with an Ice::SecurityException.
         *
         * @remarks The client certificate chain is validated using the trust object passed to the callback. When this
         * callback is set, it replaces the default validation callback and must perform all necessary validation
         * steps. If trustedRootCertificates is set, the passed trust object will use them as the anchor certificates
         * for evaluating trust. This setting can be modified by the application using
         * [SecTrustSetAnchorCertificates](https://developer.apple.com/documentation/security/1396098-sectrustsetanchorcertificates?language=objc)
         *
         * Example of setting clientCertificateValidationCallback:
         * ```cpp
         * communicator->createObjectAdapterWithEndpoints(
         *   "Hello",
         *   "ssl -h 127.0.0.1 -p 10000",
         *   ServerAuthenticationOptions{
         *     // Require client certificate
         *     .clientCertificateRequired = kAlwaysAuthenticate,
         *     .clientCertificateValidationCallback =
         *       [](SecTrustRef trust, const IceSSL::ConnectionInfoPtr& info)
         *       {
         *          ...
         *          return SecTrustEvaluateWithError(trust, nullptr);
         *       }
         * });
         * ```
         *
         * @param trust The trust object that contains the client's certificate chain.
         * @param info The IceSSL::ConnectionInfoPtr object that provides additional connection-related data which might
         * be relevant for contextual certificate validation.
         * @return true if the certificate chain is valid and the connection should proceed; false if the certificate
         * chain is invalid and the connection should be aborted.
         * @throws Ice::SecurityException if the certificate chain is invalid and the connection should be aborted.
         *
         * [See
         * SecTrustEvaluateWithError](https://developer.apple.com/documentation/security/2980705-sectrustevaluatewitherror?language=objc)
         */
        std::function<bool(SecTrustRef trust, const IceSSL::ConnectionInfoPtr& info)>
            clientCertificateValidationCallback;
#else // ICE_USE_OPENSSL
        /**
         * A callback that allows selecting the server's SSL_CTX object based on the name of the object adapter that
         * accepted the connection.
         *
         * @remarks This callback is used to associate a specific SSL configuration with an incoming connection
         * identified by the name of the object adapter that accepted it. The returned SSL_CTX pointer is wrapped in an
         * SslContextPtr object, which ensures that the SSL_CTX object is not destroyed until the caller releases it.
         * The returned SslContextPtr object must hold a pointer to a valid SSL_CTX object which was previously
         * initialized using OpenSSL API.
         *
         * If the application does not provide a callback, the Ice SSL transport will use a SSL_CTX object created
         * with SSL_CTX_new() for the connection, which uses the systems default OpenSSL configuration.
         *
         * The SSL transports calls this callback for each new incomming connection it accepts to obtain the SSL_CTX
         * object before it starts the SSL handshake.
         *
         * @param adapterName The name of the object adapter that accepted the connection.
         * @return A SslContextPtr that holds the SSL_CTX representing the SSL configuration for the adapter with the
         * specified adapterName.
         *
         * @see Detailed OpenSSL documentation on SSL_CTX management:
         * https://www.openssl.org/docs/manmaster/man3/SSL_CTX_new.html
         */
        std::function<SSL_CTX*(const std::string& adapterName)> serverSslContextSelectionCallback;

        /**
         * A callback that is invoked before initiating a new SSL handshake. This callback provides an opportunity to
         * customize the SSL parameters for the session based on specific server settings or requirements.
         *
         * @param ssl A pointer to the SSL object representing the connection.
         * @param adapterName The name of the object adapter that accepted the connection.
         *
         * @see Detailed OpenSSL documentation on SSL object management:
         * https://www.openssl.org/docs/manmaster/man3/SSL_new.html
         */
        std::function<void(::SSL* ssl, const std::string& adapterName)> sslNewSessionCallback;

        /**
         * A callback that allows manually validating the client certificate chain. When the verification callback
         * returns false, the connection will be aborted with an Ice::SecurityException.
         *
         * @param verified A boolean indicating whether the preliminary certificate verification done by OpenSSL's
         * built-in mechanisms succeeded or failed. True if the preliminary checks passed, false otherwise.
         * @param ctx A pointer to an X509_STORE_CTX object, which contains the certificate chain to be verified.
         * @param info The IceSSL::ConnectionInfoPtr object that provides additional connection-related data
         * which might be relevant for contextual certificate validation.
         * @return true if the certificate chain is valid and the connection should proceed; false if the certificate
         * chain is invalid and the connection should be aborted. An exception may be thrown to indicate a custom
         * error during the validation process.
         *
         * @note Throwing an exception from this callback will result in the termination of the connection.
         *
         * @see More details on certificate verification in OpenSSL:
         * https://www.openssl.org/docs/manmaster/man3/SSL_set_verify.html
         * @see More about X509_STORE_CTX management:
         * https://www.openssl.org/docs/manmaster/man3/X509_STORE_CTX_new.html
         */
        std::function<bool(bool verified, X509_STORE_CTX* ctx, const IceSSL::ConnectionInfoPtr& info)>
            clientCertificateValidationCallback;

#endif
    };
}

#endif
