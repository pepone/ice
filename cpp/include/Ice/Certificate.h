//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#ifndef ICESSL_PLUGIN_H
#define ICESSL_PLUGIN_H

#include "Config.h"
#include "Exception.h"
#include "Plugin.h"
#include "SSLConnectionInfoF.h"

#include <chrono>
#include <cstdint>
#include <functional>
#include <list>
#include <vector>

namespace IceSSL
{
    /**
     * The reason for an IceSSL certificate verification failure.
     */
    enum class TrustError : std::uint8_t
    {
        /** The certification verification succeed  */
        NoError = 0,
        /** The certificate chain length is greater than the specified maximum depth **/
        ChainTooLong,
        /** The X509 chain is invalid because a certificate has excluded a name constraint **/
        HasExcludedNameConstraint,
        /** The certificate has an undefined name constraint **/
        HasNonDefinedNameConstraint,
        /** The certificate has a non permitted name constraint **/
        HasNonPermittedNameConstraint,
        /** The certificate does not support a critical extension **/
        HasNonSupportedCriticalExtension,
        /** The certificate does not have a supported name constraint or has a name constraint that is unsupported **/
        HasNonSupportedNameConstraint,
        /** A host name mismatch has occurred **/
        HostNameMismatch,
        /** The X509 chain is invalid due to invalid basic constraints **/
        InvalidBasicConstraints,
        /** The X509 chain is invalid due to an invalid extension **/
        InvalidExtension,
        /** The X509 chain is invalid due to invalid name constraints **/
        InvalidNameConstraints,
        /** The X509 chain is invalid due to invalid policy constraints **/
        InvalidPolicyConstraints,
        /** The supplied certificate cannot be used for the specified purpose **/
        InvalidPurpose,
        /** The X509 chain is invalid due to an invalid certificate signature **/
        InvalidSignature,
        /** The X509 chain is not valid due to an invalid time value, such as a value that indicates an expired
            certificate **/
        InvalidTime,
        /** The certificate is explicitly not trusted **/
        NotTrusted,
        /** The X509 chain could not be built up to the root certificate **/
        PartialChain,
        /** It is not possible to determine whether the certificate has been revoked **/
        RevocationStatusUnknown,
        /** The X509 chain is invalid due to a revoked certificate **/
        Revoked,
        /** The X509 chain is invalid due to an untrusted root certificate **/
        UntrustedRoot,
        /** The X509 chain is invalid due to other unknown failure **/
        UnknownTrustFailure,
    };

    ICE_API std::string getTrustErrorDescription(TrustError);

    /**
     * The key usage "digitalSignature" bit is set
     */
    const unsigned int KEY_USAGE_DIGITAL_SIGNATURE = 1u << 0;
    /**
     * The key usage "nonRepudiation" bit is set
     */
    const unsigned int KEY_USAGE_NON_REPUDIATION = 1u << 1;
    /**
     * The key usage "keyEncipherment" bit is set
     */
    const unsigned int KEY_USAGE_KEY_ENCIPHERMENT = 1u << 2;
    /**
     * The key usage "dataEncipherment" bit is set
     */
    const unsigned int KEY_USAGE_DATA_ENCIPHERMENT = 1u << 3;
    /**
     * The key usage "keyAgreement" bit is set
     */
    const unsigned int KEY_USAGE_KEY_AGREEMENT = 1u << 4;
    /**
     * The key usage "keyCertSign" bit is set
     */
    const unsigned int KEY_USAGE_KEY_CERT_SIGN = 1u << 5;
    /**
     * The key usage "cRLSign" bit is set
     */
    const unsigned int KEY_USAGE_CRL_SIGN = 1u << 6;
    /**
     * The key usage "encipherOnly" bit is set
     */
    const unsigned int KEY_USAGE_ENCIPHER_ONLY = 1u << 7;
    /**
     * The key usage "decipherOnly" bit is set
     */
    const unsigned int KEY_USAGE_DECIPHER_ONLY = 1u << 8;
    /**
     * The extended key usage "anyKeyUsage" bit is set
     */
    const unsigned int EXTENDED_KEY_USAGE_ANY_KEY_USAGE = 1u << 0;
    /**
     * The extended key usage "serverAuth" bit is set
     */
    const unsigned int EXTENDED_KEY_USAGE_SERVER_AUTH = 1u << 1;
    /**
     * The extended key usage "clientAuth" bit is set
     */
    const unsigned int EXTENDED_KEY_USAGE_CLIENT_AUTH = 1u << 2;
    /**
     * The extended key usage "codeSigning" bit is set
     */
    const unsigned int EXTENDED_KEY_USAGE_CODE_SIGNING = 1u << 3;
    /**
     * The extended key usage "emailProtection" bit is set
     */
    const unsigned int EXTENDED_KEY_USAGE_EMAIL_PROTECTION = 1u << 4;
    /**
     * The extended key usage "timeStamping" bit is set
     */
    const unsigned int EXTENDED_KEY_USAGE_TIME_STAMPING = 1u << 5;
    /**
     * The extended key usage "OCSPSigning" bit is set
     */
    const unsigned int EXTENDED_KEY_USAGE_OCSP_SIGNING = 1u << 6;

    /**
     * Thrown if the certificate cannot be read.
     */
    class ICE_API CertificateReadException : public Ice::Exception
    {
    public:
        using Ice::Exception::Exception;

        CertificateReadException(const char*, int, std::string) noexcept;

        std::string ice_id() const override;

        /** The reason for the exception. */
        std::string reason;

    private:
        static const char* _name;
    };

    /**
     * Thrown if the certificate cannot be encoded.
     */
    class ICE_API CertificateEncodingException : public Ice::Exception
    {
    public:
        using Ice::Exception::Exception;

        CertificateEncodingException(const char*, int, std::string) noexcept;

        std::string ice_id() const override;

        /** The reason for the exception. */
        std::string reason;

    private:
        static const char* _name;
    };

    /**
     * This exception is thrown if a distinguished name cannot be parsed.
     */
    class ICE_API ParseException : public Ice::Exception
    {
    public:
        using Ice::Exception::Exception;

        ParseException(const char*, int, std::string) noexcept;

        std::string ice_id() const override;

        /** The reason for the exception. */
        std::string reason;

    private:
        static const char* _name;
    };

    /**
     * This class represents a DistinguishedName, similar to the Java
     * type X500Principal and the .NET type X500DistinguishedName.
     *
     * For comparison purposes, the value of a relative distinguished
     * name (RDN) component is always unescaped before matching,
     * therefore "ZeroC, Inc." will match ZeroC\, Inc.
     *
     * toString() always returns exactly the same information as was
     * provided in the constructor (i.e., "ZeroC, Inc." will not turn
     * into ZeroC\, Inc.).
     */
    class ICE_API DistinguishedName
    {
    public:
        /**
         * Creates a DistinguishedName from a string encoded using the rules in RFC2253.
         * @param name The encoded distinguished name.
         * @throws ParseException if parsing fails.
         */
        explicit DistinguishedName(const std::string& name);

        /**
         * Creates a DistinguishedName from a list of RDN pairs,
         * where each pair consists of the RDN's type and value.
         * For example, the RDN "O=ZeroC" is represented by the
         * pair ("O", "ZeroC").
         * @throws ParseException if parsing fails.
         */
        explicit DistinguishedName(const std::list<std::pair<std::string, std::string>>&);

        /**
         * Performs an exact match. The order of the RDN components is important.
         */
        friend ICE_API bool operator==(const DistinguishedName&, const DistinguishedName&);

        /**
         * Performs an exact match. The order of the RDN components is important.
         */
        friend ICE_API bool operator<(const DistinguishedName&, const DistinguishedName&);

        /**
         * Performs a partial match with another DistinguishedName.
         * @param dn The name to be matched.
         * @return True if all of the RDNs in the argument are present in this
         * DistinguishedName and they have the same values.
         */
        bool match(const DistinguishedName& dn) const;

        /**
         * Performs a partial match with another DistinguishedName.
         * @param dn The name to be matched.
         * @return True if all of the RDNs in the argument are present in this
         * DistinguishedName and they have the same values.
         */
        bool match(const std::string& dn) const;

        /**
         * Encodes the DN in RFC2253 format.
         * @return An encoded string.
         */
        std::string toString() const;

        /**
         * Encodes the DN in RFC2253 format.
         * @return An encoded string.
         */
        operator std::string() const { return toString(); }

    protected:
        /// \cond INTERNAL
        void unescape();
        /// \endcond

    private:
        std::list<std::pair<std::string, std::string>> _rdns;
        std::list<std::pair<std::string, std::string>> _unescaped;
    };

    /**
     * Performs an exact match. The order of the RDN components is important.
     */
    inline bool operator>(const DistinguishedName& lhs, const DistinguishedName& rhs) { return rhs < lhs; }

    /**
     * Performs an exact match. The order of the RDN components is important.
     */
    inline bool operator<=(const DistinguishedName& lhs, const DistinguishedName& rhs) { return !(lhs > rhs); }

    /**
     * Performs an exact match. The order of the RDN components is important.
     */
    inline bool operator>=(const DistinguishedName& lhs, const DistinguishedName& rhs) { return !(lhs < rhs); }

    /**
     * Performs an exact match. The order of the RDN components is important.
     */
    inline bool operator!=(const DistinguishedName& lhs, const DistinguishedName& rhs) { return !(lhs == rhs); }

    /**
     * Represents an X509 Certificate extension.
     */
    class ICE_API X509Extension
    {
    public:
        /**
         * Determines whether the information in this extension is important.
         * @return True if if information is important, false otherwise.
         */
        virtual bool isCritical() const = 0;

        /**
         * Obtains the object ID of this extension.
         * @return The object ID.
         */
        virtual std::string getOID() const = 0;

        /**
         * Obtains the data associated with this extension.
         * @return The extension data.
         */
        virtual std::vector<std::uint8_t> getData() const = 0;
    };
    using X509ExtensionPtr = std::shared_ptr<X509Extension>;

    class Certificate;
    using CertificatePtr = std::shared_ptr<Certificate>;

    /**
     * This convenience class is a wrapper around a native certificate.
     * The interface is inspired by java.security.cert.X509Certificate.
     */
    class ICE_API Certificate : public std::enable_shared_from_this<Certificate>
    {
    public:
        /**
         * Compares the certificates for equality using the native certificate comparison method.
         */
        virtual bool operator==(const Certificate&) const = 0;

        /**
         * Compares the certificates for equality using the native certificate comparison method.
         */
        virtual bool operator!=(const Certificate&) const = 0;

        /**
         * Obtains the authority key identifier.
         * @return The identifier.
         */
        virtual std::vector<std::uint8_t> getAuthorityKeyIdentifier() const = 0;

        /**
         * Obtains the subject key identifier.
         * @return The identifier.
         */
        virtual std::vector<std::uint8_t> getSubjectKeyIdentifier() const = 0;

        /**
         * Verifies that this certificate was signed by the given certificate
         * public key.
         * @param cert A certificate containing the public key.
         * @return True if signed, false otherwise.
         */
        virtual bool verify(const CertificatePtr& cert) const = 0;

        /**
         * Obtains a string encoding of the certificate in PEM format.
         * @return The encoded certificate.
         * @throws CertificateEncodingException if an error occurs.
         */
        virtual std::string encode() const = 0;

        /**
         * Checks that the certificate is currently valid, that is, the current
         * date falls between the validity period given in the certificate.
         * @return True if the certificate is valid, false otherwise.
         */
        virtual bool checkValidity() const = 0;

        /**
         * Checks that the certificate is valid at the given time.
         * @param t The target time.
         * @return True if the certificate is valid, false otherwise.
         */
        virtual bool checkValidity(const std::chrono::system_clock::time_point& t) const = 0;

        /**
         * Returns the value of the key usage extension. The flags <b>KEY_USAGE_DIGITAL_SIGNATURE</b>,
         * <b>KEY_USAGE_NON_REPUDIATION</b>, <b>KEY_USAGE_KEY_ENCIPHERMENT</b>, <b>KEY_USAGE_DATA_ENCIPHERMENT</b>
         * <b>KEY_USAGE_KEY_AGREEMENT</b>, <b>KEY_USAGE_KEY_CERT_SIGN</b>, <b>KEY_USAGE_CRL_SIGN</b>,
         * <b>KEY_USAGE_ENCIPHER_ONLY</b> and <b>KEY_USAGE_DECIPHER_ONLY</b> can be used to check what
         * key usage bits are set.
         */
        virtual unsigned int getKeyUsage() const = 0;

        /**
         * Returns the value of the extended key usage extension. The flags <b>EXTENDED_KEY_USAGE_ANY_KEY_USAGE</b>,
         * <b>EXTENDED_KEY_USAGE_SERVER_AUTH</b>, <b>EXTENDED_KEY_USAGE_CLIENT_AUTH</b>,
         * <b>EXTENDED_KEY_USAGE_CODE_SIGNING</b>, <b>EXTENDED_KEY_USAGE_EMAIL_PROTECTION</b>,
         * <b>EXTENDED_KEY_USAGE_TIME_STAMPING</b> and <b>EXTENDED_KEY_USAGE_OCSP_SIGNING</b> can be used to check what
         * extended key usage bits are set.
         */
        virtual unsigned int getExtendedKeyUsage() const = 0;

        /**
         * Obtains the not-after validity time.
         * @return The time after which this certificate is invalid.
         */
        virtual std::chrono::system_clock::time_point getNotAfter() const = 0;

        /**
         * Obtains the not-before validity time.
         * @return The time at which this certificate is valid.
         */
        virtual std::chrono::system_clock::time_point getNotBefore() const = 0;

        /**
         * Obtains the serial number. This is an arbitrarily large number.
         * @return The certificate's serial number.
         */
        virtual std::string getSerialNumber() const = 0;

        /**
         * Obtains the issuer's distinguished name (DN).
         * @return The distinguished name.
         */
        virtual DistinguishedName getIssuerDN() const = 0;

        /**
         * Obtains the values in the issuer's alternative names extension.
         *
         * The returned list contains a pair of int, string.
         *
         * otherName                       [0]     OtherName
         * rfc822Name                      [1]     IA5String
         * dNSName                         [2]     IA5String
         * x400Address                     [3]     ORAddress
         * directoryName                   [4]     Name
         * ediPartyName                    [5]     EDIPartyName
         * uniformResourceIdentifier       [6]     IA5String
         * iPAddress                       [7]     OCTET STRING
         * registeredID                    [8]     OBJECT IDENTIFIER
         *
         * rfc822Name, dNSName, directoryName and
         * uniformResourceIdentifier data is returned as a string.
         *
         * iPAddress is returned in dotted quad notation. IPv6 is not
         * currently supported.
         *
         * All distinguished names are encoded in RFC2253 format.
         *
         * The remainder of the data will result in an empty string. Use the raw
         * X509* certificate to obtain these values.
         *
         * @return The issuer's alternative names.
         */
        virtual std::vector<std::pair<int, std::string>> getIssuerAlternativeNames() const = 0;

        /**
         * Obtains the subject's distinguished name (DN).
         * @return The distinguished name.
         */
        virtual DistinguishedName getSubjectDN() const = 0;

        /**
         * See the comment for Plugin::getIssuerAlternativeNames.
         * @return The subject's alternative names.
         */
        virtual std::vector<std::pair<int, std::string>> getSubjectAlternativeNames() const = 0;

        /**
         * Obtains the certificate version number.
         * @return The version number.
         */
        virtual int getVersion() const = 0;

        /**
         * Stringifies the certificate. This is a human readable version of
         * the certificate, not a DER or PEM encoding.
         * @return A string version of the certificate.
         */
        virtual std::string toString() const = 0;

        /**
         * Obtains a list of the X509v3 extensions contained in the certificate.
         * @return The extensions.
         */
        virtual std::vector<X509ExtensionPtr> getX509Extensions() const = 0;

        /**
         * Obtains the extension with the given OID.
         * @return The extension, or null if the certificate
         * does not contain a extension with the given OID.
         */
        virtual X509ExtensionPtr getX509Extension(const std::string& oid) const = 0;

        /**
         * Loads the certificate from a file. The certificate must use the
         * PEM encoding format.
         * @param file The certificate file.
         * @return The new certificate instance.
         * @throws CertificateReadException if the file cannot be read.
         */
        static CertificatePtr load(const std::string& file);

        /**
         * Decodes a certificate from a string that uses the PEM encoding format.
         * @param str A string containing the encoded certificate.
         * @throws CertificateEncodingException if an error occurs.
         */
        static CertificatePtr decode(const std::string& str);
    };
}

#endif
