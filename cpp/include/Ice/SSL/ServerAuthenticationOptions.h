//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#ifndef ICE_SSL_SERVER_AUTHENTICATION_OPTIONS_H
#define ICE_SSL_SERVER_AUTHENTICATION_OPTIONS_H

#include "Config.h"
#include "ConnectionInfo.h"

#include <functional>

namespace Ice::SSL
{
#if defined(ICE_USE_SCHANNEL)
    /**
     * The SSL configuration properties for server connections.
     */
    struct SchannelServerAuthenticationOptions
    {
        /**
         * A callback that allows selecting the server's SSL certificate based on the name of the object adapter that
         * accepts the connection.
         *
         * @remarks This callback is invoked by the SSL transport for each new incoming connection before starting the
         * SSL handshake to determine the appropriate server credentials. The callback should return a [SCH_CREDENTIALS]
         * that represents the server's credentials. The SSL transport takes ownership of the credentials' paCred and
         * and hRootStore, and releases them when the connection is closed.
         *
         * @param adapterName The name of the object adapter that accepted the connection.
         * @return The server's credentials.
         *
         * Example of setting serverCertificateSelectionCallback: TODO
         *
         * [SCH_CREDENTIALS]:
         * https://learn.microsoft.com/en-us/windows/win32/api/schannel/ns-schannel-sch_credentials
         */
        std::function<SCH_CREDENTIALS(const std::string& host)> serverCredentialsSelectionCallback;

        /**
         * A callback that is invoked before initiating a new SSL handshake. This callback provides an opportunity to
         * customize the SSL parameters for the session based on specific server settings or requirements.
         *
         * Example of setting sslNewSessionCallback: TODO
         *
         * @param context The CtxtHandle representing the security context associated with the current connection.
         * @param adapterName The name of the object adapter that accepted the connection.
         */
        std::function<void(CtxtHandle context, const std::string& adapterName)> sslNewSessionCallback;

        /**
         * Whether or not the client must provide a certificate. The default value is false.
         */
        bool clientCertificateRequired = false;

        /**
         * The trusted root certificates used for validating the client's certificate chain. If this field is set, the
         * server's certificate chain is validated against these certificates; otherwise, the system's default root
         * certificates are used.
         *
         * @remarks The trusted root certificates are only used by the default validation callback; they are ignored by
         * custom validation callbacks set with clientCertificateValidationCallback.
         *
         * The application must ensure that the certificate store remains valid during the setup of the ObjectAdapter.
         * It is also the application's responsibility to release the certificate store object after the ObjectAdapter
         * has been created to prevent memory leaks.
         *
         * Example of setting trustedRootCertificates: TODO
         */
        HCERTSTORE trustedRootCertificates = nullptr;

        /**
         * A callback that allows manually validating the client certificate chain. When the verification callback
         * returns false, the connection will be aborted with an Ice::SecurityException.
         *
         * @remarks The server certificate chain is validated using the context object passed to the callback. When this
         * callback is set, it replaces the default validation callback and must perform all necessary validation
         * steps. If trustedRootCertificates is set, the passed context object will use them as the trusted root
         * certificates for validation. This setting can be modified by the application using
         * [SecTrustSetAnchorCertificates](https://developer.apple.com/documentation/security/1396098-sectrustsetanchorcertificates?language=objc)
         *
         * Example of setting clientCertificateValidationCallback: TODO
         *
         * @param context A CtxtHandle representing the security context associated with the current connection. This
         * context contains security data relevant for validation, such as the client's certificate chain and cipher
         * suite.
         * @param info The ConnectionInfoPtr object that provides additional connection-related data which might
         * be relevant for contextual certificate validation.
         * @return true if the certificate chain is valid and the connection should proceed; false if the certificate
         * chain is invalid and the connection should be aborted.
         * @throws Ice::SecurityException if the certificate chain is invalid and the connection should be aborted.
         *
         * [See
         * SecTrustEvaluateWithError](https://developer.apple.com/documentation/security/2980705-sectrustevaluatewitherror?language=objc)
         */
        std::function<bool(CtxtHandle context, const ConnectionInfoPtr& info)> clientCertificateValidationCallback;
    };

    /// Alias for the platform-specific implementation of ClientAuthenticationOptions on Windows.
    /// \cond INTERNAL
    using ServerAuthenticationOptions = SchannelServerAuthenticationOptions;
    /// \endcond
#elif defined(ICE_USE_SECURE_TRANSPORT)
    /**
     * The SSL configuration properties for server connections.
     */
    struct SecureTransportServerAuthenticationOptions
    {
        /**
         * A callback that allows selecting the server's SSL certificate chain based on the name of the object adapter
         * that accepts the connection.
         *
         * @remarks This callback is invoked by the SSL transport for each new incoming connection before starting the
         * SSL handshake to determine the appropriate server certificate chain. The callback should return a CFArrayRef
         * that represents the server's certificate chain, or nullptr if no certificate chain should be used for the
         * connection. The SSL transport takes ownership of the returned CFArrayRef and releases it when the connection
         * is closed.
         *
         * @param adapterName The name of the object adapter that accepted the connection.
         * @return CFArrayRef containing the server's certificate chain, or nullptr to indicate that no certificate is
         * used.
         *
         * Example of setting serverCertificateSelectionCallback:
         * @snippet Ice/SSL/SecureTransportServerAuthenticationOptions.cpp serverCertificateSelectionCallback
         *
         * See the [SSLSetCertificate] documentation for requirements on the certificate chain format.
         *
         * [SSLSetCertificate]:
         * https://developer.apple.com/documentation/security/1392400-sslsetcertificate?changes=_3&language=objc
         */
        std::function<CFArrayRef(const std::string& adapterName)> serverCertificateSelectionCallback;

        /**
         * A callback that is invoked before initiating a new SSL handshake. This callback provides an opportunity to
         * customize the SSL parameters for the session based on specific server settings or requirements.
         *
         * Example of setting sslNewSessionCallback:
         * @snippet Ice/SSL/SecureTransportServerAuthenticationOptions.cpp sslNewSessionCallback
         *
         * @param context An opaque type that represents an SSL session context object.
         * @param adapterName The name of the object adapter that accepted the connection.
         */
        std::function<void(SSLContextRef context, const std::string& adapterName)> sslNewSessionCallback;

        /**
         * The requirements for client-side authentication. The default is kNeverAuthenticate.
         *
         * [see SSLAuthenticate](https://developer.apple.com/documentation/security/sslauthenticate)
         */
        SSLAuthenticate clientCertificateRequired = kNeverAuthenticate;

        /**
         * The trusted root certificates used for validating the client's certificate chain. If this field is set, the
         * client's certificate chain is validated against these certificates; otherwise, the system's default root
         * certificates are used.
         *
         * @remarks The trusted root certificates are used by both the default validation callback, and by custom
         * validation callback set in clientCertificateValidationCallback.
         *
         * This is equivalent to calling [SecTrustSetAnchorCertificates] with the CFArrayRef object; and
         * [SecTrustSetAnchorCertificatesOnly] with the `anchorCertificatesOnly` parameter set to true.
         *
         * Example of setting trustedRootCertificates:
         * @snippet Ice/SSL/SecureTransportServerAuthenticationOptions.cpp trustedRootCertificates
         *
         * [SecTrustSetAnchorCertificates]:
         * https://developer.apple.com/documentation/security/1396098-sectrustsetanchorcertificates?language=objc
         * [SecTrustSetAnchorCertificatesOnly]:
         * https://developer.apple.com/documentation/security/1399071-sectrustsetanchorcertificatesonl?language=objc
         */
        CFArrayRef trustedRootCertificates = nullptr;

        /**
         * A callback that allows manually validating the client certificate chain. When the verification callback
         * returns false, the connection will be aborted with an Ice::SecurityException.
         *
         * @remarks The client certificate chain is validated using the trust object passed to the callback. When this
         * callback is set, it replaces the default validation callback and must perform all necessary validation
         * steps. If trustedRootCertificates is set, the passed trust object will use them as the anchor certificates
         * for evaluating trust. This setting can be modified by the application using [SecTrustSetAnchorCertificates].
         *
         * Example of setting clientCertificateValidationCallback:
         * @snippet Ice/SSL/SecureTransportServerAuthenticationOptions.cpp clientCertificateValidationCallback
         *
         * @param trust The trust object that contains the client's certificate chain.
         * @param info The ConnectionInfoPtr object that provides additional connection-related data which might
         * be relevant for contextual certificate validation.
         * @return true if the certificate chain is valid and the connection should proceed; false if the certificate
         * chain is invalid and the connection should be aborted.
         * @throws Ice::SecurityException if the certificate chain is invalid and the connection should be aborted.
         *
         * @see [SecTrustEvaluateWithError]
         *
         * [SecTrustSetAnchorCertificates]:
         * https://developer.apple.com/documentation/security/1396098-sectrustsetanchorcertificates?language=objc
         * [SecTrustEvaluateWithError]:
         * https://developer.apple.com/documentation/security/2980705-sectrustevaluatewitherror?language=objc
         */
        std::function<bool(SecTrustRef trust, const ConnectionInfoPtr& info)> clientCertificateValidationCallback;
    };

    /// Alias for the platform-specific implementation of ClientAuthenticationOptions on macOS and iOS.
    /// \cond INTERNAL
    using ServerAuthenticationOptions = SecureTransportServerAuthenticationOptions;
    /// \endcond
#elif defined(ICE_USE_OPENSSL)
    /**
     * The SSL configuration properties for server connections.
     */
    struct OpenSSLServerAuthenticationOptions
    {
        /**
         * A callback that allows selecting the server's SSL_CTX object based on the name of the object adapter that
         * accepted the connection.
         *
         * @remarks This callback is used to associate a specific SSL configuration with an incoming connection
         * identified by the name of the object adapter that accepted the connection. The callback must return a pointer
         * to a valid SSL_CTX object which was previously initialized using OpenSSL API. The SSL transport takes
         * ownership of the returned SSL_CTX pointer and releases it after closing the connection.
         *
         * If the application does not provide a callback, the Ice SSL transport will use a SSL_CTX object created
         * with SSL_CTX_new() for the connection, which uses the systems default OpenSSL configuration.
         *
         * The SSL transports calls this callback for each new incoming connection to obtain the SSL_CTX object before
         * it starts the SSL handshake.
         *
         * @param adapterName The name of the object adapter that accepted the connection.
         * @return A pointer to a SSL_CTX objet representing the SSL configuration for the new incoming connection.
         *
         * Example of setting serverSSLContextSelectionCallback:
         * @snippet Ice/SSL/OpenSSLServerAuthenticationOptions.cpp serverSSLContextSelectionCallback
         *
         * @see Detailed OpenSSL documentation on SSL_CTX management:
         * https://www.openssl.org/docs/manmaster/man3/SSL_CTX_new.html
         */
        std::function<SSL_CTX*(const std::string& adapterName)> serverSSLContextSelectionCallback;

        /**
         * A callback that is invoked before initiating a new SSL handshake. This callback provides an opportunity to
         * customize the SSL parameters for the connection.
         *
         * @param ssl A pointer to the SSL object representing the connection.
         * @param adapterName The name of the object adapter that accepted the connection.
         *
         * Example of setting sslNewSessionCallback:
         * @snippet Ice/SSL/OpenSSLServerAuthenticationOptions.cpp sslNewSessionCallback
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
         * @param info The ConnectionInfoPtr object that provides additional connection-related data
         * which might be relevant for contextual certificate validation.
         * @return true if the certificate chain is valid and the connection should proceed; false if the certificate
         * chain is invalid and the connection should be aborted.
         * @throws Ice::SecurityException if the certificate chain is invalid and the connection should be aborted.
         *
         * Example of setting clientCertificateValidationCallback:
         * @snippet Ice/SSL/OpenSSLServerAuthenticationOptions.cpp clientCertificateValidationCallback
         *
         * @see More details on certificate verification in OpenSSL:
         * https://www.openssl.org/docs/manmaster/man3/SSL_set_verify.html
         * @see More about X509_STORE_CTX management:
         * https://www.openssl.org/docs/manmaster/man3/X509_STORE_CTX_new.html
         */
        std::function<bool(bool verified, X509_STORE_CTX* ctx, const ConnectionInfoPtr& info)>
            clientCertificateValidationCallback;
    };

    /// Alias for the platform-specific implementation of ClientAuthenticationOptions on Linux.
    /// \cond INTERNAL
    using ServerAuthenticationOptions = OpenSSLServerAuthenticationOptions;
    /// \endcond
#else
#    error "unsupported platform"
#endif

}

#endif
