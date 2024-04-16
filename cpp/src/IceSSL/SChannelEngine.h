//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#ifndef ICESSL_SCHANNEL_ENGINE_H
#define ICESSL_SCHANNEL_ENGINE_H

#ifdef _WIN32

#    include "Ice/InstanceF.h"
#    include "SChannelEngineF.h"
#    include "SSLEngine.h"

#    include <string>
#    include <vector>

//
// SECURITY_WIN32 or SECURITY_KERNEL, must be defined before including security.h
// indicating who is compiling the code.
//
#    ifdef SECURITY_WIN32
#        undef SECURITY_WIN32
#    endif
#    ifdef SECURITY_KERNEL
#        undef SECURITY_KERNEL
#    endif
#    define SECURITY_WIN32 1
#    include <schannel.h>
#    include <security.h>
#    include <sspi.h>
#    undef SECURITY_WIN32

namespace IceSSL::SChannel
{
    class SSLEngine final : public IceSSL::SSLEngine
    {
    public:
        SSLEngine(const IceInternal::InstancePtr&);

        //
        // Setup the engine.
        //
        void initialize() final;

        IceInternal::TransceiverPtr
        createTransceiver(const InstancePtr&, const IceInternal::TransceiverPtr&, const std::string&, bool) final;

        //
        // Destroy the engine.
        //
        void destroy() final;

        std::string getCipherName(ALG_ID) const;

        CredHandle newCredentialsHandle(bool);

        HCERTCHAINENGINE chainEngine() const;

    private:
        std::vector<PCCERT_CONTEXT> _allCerts;
        std::vector<PCCERT_CONTEXT> _importedCerts;

        std::vector<HCERTSTORE> _stores;
        HCERTSTORE _rootStore;

        HCERTCHAINENGINE _chainEngine;
        std::vector<ALG_ID> _ciphers;

        const bool _strongCrypto;
    };
}

#endif

#endif
