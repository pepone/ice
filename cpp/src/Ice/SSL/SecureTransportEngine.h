// Copyright (c) ZeroC, Inc.

#ifndef ICE_SSL_SECURE_TRANSPORT_ENGINE_H
#define ICE_SSL_SECURE_TRANSPORT_ENGINE_H

#ifdef __APPLE__

#    include "../UniqueRef.h"
#    include "Ice/InstanceF.h"
#    include "Ice/SSL/ClientAuthenticationOptions.h"
#    include "Ice/SSL/ServerAuthenticationOptions.h"
#    include "SSLEngine.h"

#    include <Security/SecureTransport.h>
#    include <Security/Security.h>
#    include <vector>

namespace Ice::SSL::SecureTransport
{
    class SSLEngine final : public Ice::SSL::SSLEngine, public std::enable_shared_from_this<SSLEngine>
    {
    public:
        SSLEngine(const IceInternal::InstancePtr&);
        ~SSLEngine();

        void initialize() final;
        void destroy() final;

        [[nodiscard]] Ice::SSL::ClientAuthenticationOptions
        createClientAuthenticationOptions(const std::string& host) const final;
        [[nodiscard]] Ice::SSL::ServerAuthenticationOptions createServerAuthenticationOptions() const final;
        [[nodiscard]] SSLContextRef newContext(bool) const;
        [[nodiscard]] bool
        validationCallback(SecTrustRef trust, const Ice::SSL::ConnectionInfoPtr&, const std::string&) const;

        [[nodiscard]] std::string getCipherName(SSLCipherSuite) const;

    private:
        IceInternal::UniqueRef<CFArrayRef> _certificateAuthorities;
        IceInternal::UniqueRef<CFArrayRef> _chain;

        // The cipher suites enabled on every SSL context, in preference order.
        std::vector<SSLCipherSuite> _ciphers;

        // Path of the temporary keychain holding the imported certificate, removed by destroy().
        // Empty when IceSSL.Keychain is set or no certificate is configured.
        std::string _temporaryKeychainPath;
    };
}
#endif

#endif
