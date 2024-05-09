//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#ifndef ICE_SSL_SSL_UTIL_H
#define ICE_SSL_SSL_UTIL_H

#include "DistinguishedName.h"
#include "Ice/Config.h"
#include "Ice/SSL/Config.h"

#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace Ice::SSL
{
#if defined(__APPLE__)
    //
    // Helper functions to use by Secure Transport.
    //
    std::string fromCFString(CFStringRef);

    inline CFStringRef toCFString(const std::string& s)
    {
        return CFStringCreateWithCString(nullptr, s.c_str(), kCFStringEncodingUTF8);
    }
#endif

    // Constants for X509 certificate alt names (AltNameOther, AltNameORAddress, AltNameEDIPartyName and
    // AltNameObjectIdentifier) are not supported.

    // const int AltNameOther = 0;
    const int AltNameEmail = 1;
    const int AltNameDNS = 2;
    // const int AltNameORAddress = 3;
    const int AltNameDirectory = 4;
    // const int AltNameEDIPartyName = 5;
    const int AltNameURL = 6;
    const int AltNAmeIP = 7;
    // const AltNameObjectIdentifier = 8;

    // Read a file into memory buffer.
    void readFile(const std::string&, std::vector<char>&);

    // Determine if a file or directory exists, with an optional default directory.
    bool checkPath(const std::string&, const std::string&, bool, std::string&);

    bool parseBytes(const std::string&, std::vector<unsigned char>&);

#if defined(ICE_USE_SCHANNEL)
    ICE_API DistinguishedName getSubjectName(PCCERT_CONTEXT);
    ICE_API std::vector<std::pair<int, std::string>> getSubjectAltNames(PCCERT_CONTEXT);
    ICE_API std::string encodeCertificate(PCCERT_CONTEXT);

    class ICE_API ScopedCertificate
    {
    public:
        ScopedCertificate(PCCERT_CONTEXT certificate) : _certificate(certificate) {}
        ~ScopedCertificate();
        PCCERT_CONTEXT get() const { return _certificate; }

    private:
        PCCERT_CONTEXT _certificate;
    };
    ICE_API PCCERT_CONTEXT decodeCertificate(const std::string&);
#elif defined(ICE_USE_SECURE_TRANSPORT)
    std::string certificateOIDAlias(const std::string&);
    ICE_API DistinguishedName getSubjectName(SecCertificateRef);
    ICE_API std::vector<std::pair<int, std::string>> getSubjectAltNames(SecCertificateRef);
    ICE_API std::string encodeCertificate(SecCertificateRef);

    class ICE_API ScopedCertificate
    {
    public:
        ScopedCertificate(SecCertificateRef certificate) : _certificate(certificate) {}
        ~ScopedCertificate();
        SecCertificateRef get() const { return _certificate; }

    private:
        SecCertificateRef _certificate;
    };
    ICE_API SecCertificateRef decodeCertificate(const std::string&);
#elif defined(ICE_USE_OPENSSL)
    // Accumulate the OpenSSL error stack into a string.
    std::string getErrors(bool);
    ICE_API DistinguishedName getSubjectName(X509*);
    ICE_API std::vector<std::pair<int, std::string>> getSubjectAltNames(X509*);
    ICE_API std::string encodeCertificate(X509*);
    class ICE_API ScopedCertificate
    {
    public:
        ScopedCertificate(X509* certificate) : _certificate(certificate) {}
        ~ScopedCertificate();
        X509* get() const { return _certificate; }

    private:
        X509* _certificate;
    };
    ICE_API X509* decodeCertificate(const std::string&);
#endif
}

#endif
