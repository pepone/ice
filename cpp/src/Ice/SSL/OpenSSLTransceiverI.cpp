//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#include "OpenSSLTransceiverI.h"
#include "Ice/Buffer.h"
#include "Ice/Communicator.h"
#include "Ice/LocalException.h"
#include "Ice/LoggerUtil.h"
#include "Ice/SSL/ConnectionInfo.h"
#include "Ice/SSL/OpenSSL.h"
#include "OpenSSLEngine.h"
#include "SSLEngine.h"
#include "SSLInstance.h"
#include "SSLUtil.h"

#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/ssl.h>

#include <iostream>
#include <mutex>

using namespace std;
using namespace Ice;
using namespace Ice::SSL;

extern "C"
{
    int Ice_SSL_opensslVerifyCallback(int ok, X509_STORE_CTX* ctx)
    {
        ::SSL* ssl = reinterpret_cast<::SSL*>(X509_STORE_CTX_get_ex_data(ctx, SSL_get_ex_data_X509_STORE_CTX_idx()));
        OpenSSL::TransceiverI* p = reinterpret_cast<OpenSSL::TransceiverI*>(SSL_get_ex_data(ssl, 0));
        return p->verifyCallback(ok, ctx);
    }
}

namespace
{
    std::function<bool(bool, X509_STORE_CTX* ctx, const Ice::SSL::ConnectionInfoPtr&)>
    createDefaultVerificationCallback()
    {
        return [](bool, X509_STORE_CTX* ctx, const Ice::SSL::ConnectionInfoPtr&)
        {
            ::SSL* ssl = static_cast<::SSL*>(X509_STORE_CTX_get_ex_data(ctx, SSL_get_ex_data_X509_STORE_CTX_idx()));
            long result = SSL_get_verify_result(ssl);
            if (result != X509_V_OK)
            {
                ostringstream os;
                os << "IceSSL: certificate verification failed:\n" << X509_verify_cert_error_string(result);
                throw SecurityException(__FILE__, __LINE__, os.str());
            }
            return true;
        };
    }
}

IceInternal::NativeInfoPtr
OpenSSL::TransceiverI::getNativeInfo()
{
    return _delegate->getNativeInfo();
}

IceInternal::SocketOperation
OpenSSL::TransceiverI::initialize(IceInternal::Buffer& readBuffer, IceInternal::Buffer& writeBuffer)
{
    if (!_connected)
    {
        IceInternal::SocketOperation status = _delegate->initialize(readBuffer, writeBuffer);
        if (status != IceInternal::SocketOperationNone)
        {
            return status;
        }
        _connected = true;
    }

    if (!_ssl)
    {
        SOCKET fd = _delegate->getNativeInfo()->fd();
        BIO* bio = nullptr;
        if (fd == INVALID_SOCKET)
        {
            assert(_sentBytes == 0);
            _maxSendPacketSize = 128 * 1024; // 128KB
            _maxRecvPacketSize = 128 * 1024; // 128KB
            if (!BIO_new_bio_pair(&bio, _maxSendPacketSize, &_memBio, _maxRecvPacketSize))
            {
                bio = nullptr;
                _memBio = nullptr;
            }
        }
        else
        {
            bio = BIO_new_socket(fd, 0);
        }

        if (!bio)
        {
            throw SecurityException(__FILE__, __LINE__, "openssl failure");
        }

        _sslCtx = _localSSLContextSelectionCallback(_incoming ? _adapterName : _host);
        if (!_sslCtx)
        {
            throw SecurityException(__FILE__, __LINE__, "SSL error: the SSL context selection callback returned null");
        }
        _ssl = SSL_new(_sslCtx);
        if (!_ssl)
        {
            BIO_free(bio);
            if (_memBio)
            {
                BIO_free(_memBio);
                _memBio = nullptr;
            }
            throw SecurityException(__FILE__, __LINE__, "openssl failure");
        }
        SSL_set_bio(_ssl, bio, bio);

        SSL_set_ex_data(_ssl, 0, this);
        SSL_set_verify(_ssl, SSL_get_verify_mode(_ssl), Ice_SSL_opensslVerifyCallback);

        if (_sslNewSessionCallback)
        {
            _sslNewSessionCallback(_ssl, _incoming ? _adapterName : _host);
        }
    }

    while (!SSL_is_init_finished(_ssl))
    {
        int ret = _incoming ? SSL_accept(_ssl) : SSL_connect(_ssl);
        if (_memBio && BIO_ctrl_pending(_memBio))
        {
            if (!send())
            {
                return IceInternal::SocketOperationWrite;
            }
            continue;
        }

        if (ret <= 0)
        {
            switch (SSL_get_error(_ssl, ret))
            {
                case SSL_ERROR_NONE:
                {
                    assert(SSL_is_init_finished(_ssl));
                    break;
                }
                case SSL_ERROR_ZERO_RETURN:
                {
                    throw ConnectionLostException(__FILE__, __LINE__, IceInternal::getSocketErrno());
                }
                case SSL_ERROR_WANT_READ:
                {
                    if (_memBio && receive())
                    {
                        continue;
                    }
                    return IceInternal::SocketOperationRead;
                }
                case SSL_ERROR_WANT_WRITE:
                {
                    if (_memBio && send())
                    {
                        continue;
                    }
                    return IceInternal::SocketOperationWrite;
                }
                case SSL_ERROR_SYSCALL:
                {
                    if (!_memBio)
                    {
                        if (IceInternal::interrupted())
                        {
                            break;
                        }

                        if (IceInternal::wouldBlock())
                        {
                            if (SSL_want_read(_ssl))
                            {
                                return IceInternal::SocketOperationRead;
                            }
                            else if (SSL_want_write(_ssl))
                            {
                                return IceInternal::SocketOperationWrite;
                            }

                            break;
                        }

                        if (IceInternal::connectionLost() || IceInternal::getSocketErrno() == 0)
                        {
                            throw ConnectionLostException(__FILE__, __LINE__, IceInternal::getSocketErrno());
                        }
                    }
                    throw SocketException(__FILE__, __LINE__, IceInternal::getSocketErrno());
                }
                case SSL_ERROR_SSL:
                {
#if defined(SSL_R_UNEXPECTED_EOF_WHILE_READING)
                    if (SSL_R_UNEXPECTED_EOF_WHILE_READING == ERR_GET_REASON(ERR_get_error()))
                    {
                        throw ConnectionLostException(__FILE__, __LINE__, 0);
                    }
                    else
                    {
#endif
                        ostringstream os;
                        os << "SSL error occurred for new " << (_incoming ? "incoming" : "outgoing") << " connection:\n"
                           << _delegate->toString() << "\n"
                           << _engine->sslErrors();
                        throw ProtocolException(__FILE__, __LINE__, os.str());
#if defined(SSL_R_UNEXPECTED_EOF_WHILE_READING)
                    }
#endif
                }
            }
        }
    }

    if (_verificationException)
    {
        rethrow_exception(_verificationException);
    }

    // Retrieve the certificate chain.

    // When calling on the server side the peer certificate is not included in SSL_get_peer_cert_chain and must be
    // obtabined separately using SSL_get_peer_certificate.
    if (_incoming)
    {
        X509* peerCertificate = SSL_get_peer_certificate(_ssl);
        if (peerCertificate)
        {
            _certs.clear();
            CertificatePtr cert = OpenSSL::Certificate::create(peerCertificate);
            _certs.push_back(cert);
        }
    }

    STACK_OF(X509)* chain = SSL_get_peer_cert_chain(_ssl);
    if (chain)
    {
        for (int i = 0; i < sk_X509_num(chain); ++i)
        {
            CertificatePtr cert = OpenSSL::Certificate::create(X509_dup(sk_X509_value(chain, i)));
            _certs.push_back(cert);
        }
    }

    if (_engine->securityTraceLevel() >= 1)
    {
        Trace out(_instance->logger(), _instance->traceCategory());
        out << "SSL summary for " << (_incoming ? "incoming" : "outgoing") << " connection\n";

        //
        // The const_cast is necessary because Solaris still uses OpenSSL 0.9.7.
        //
        // const SSL_CIPHER *cipher = SSL_get_current_cipher(ssl);
        SSL_CIPHER* cipher = const_cast<SSL_CIPHER*>(SSL_get_current_cipher(_ssl));
        if (!cipher)
        {
            out << "unknown cipher\n";
        }
        else
        {
            out << "cipher = " << SSL_CIPHER_get_name(cipher) << "\n";
            out << "bits = " << SSL_CIPHER_get_bits(cipher, 0) << "\n";
            out << "protocol = " << SSL_get_version(_ssl) << "\n";
        }
        out << toString();
    }

    return IceInternal::SocketOperationNone;
}

IceInternal::SocketOperation
OpenSSL::TransceiverI::closing(bool initiator, exception_ptr)
{
    // If we are initiating the connection closure, wait for the peer
    // to close the TCP/IP connection. Otherwise, close immediately.
    return initiator ? IceInternal::SocketOperationRead : IceInternal::SocketOperationNone;
}

void
OpenSSL::TransceiverI::close()
{
    if (_ssl)
    {
        int err = SSL_shutdown(_ssl);

        //
        // Call it one more time if it returned 0.
        //
        if (err == 0)
        {
            SSL_shutdown(_ssl);
        }

        SSL_free(_ssl);
        _ssl = nullptr;
    }

    if (_sslCtx)
    {
        SSL_CTX_free(_sslCtx);
        _sslCtx = nullptr;
    }

    if (_memBio)
    {
        BIO_free(_memBio);
        _memBio = nullptr;
    }

    _delegate->close();
}

IceInternal::SocketOperation
OpenSSL::TransceiverI::write(IceInternal::Buffer& buf)
{
    if (!_connected)
    {
        return _delegate->write(buf);
    }

    if (_memBio && _writeBuffer.i != _writeBuffer.b.end())
    {
        if (!send())
        {
            return IceInternal::SocketOperationWrite;
        }
    }

    if (buf.i == buf.b.end())
    {
        return IceInternal::SocketOperationNone;
    }

    //
    // It's impossible for packetSize to be more than an Int.
    //
    int packetSize = _memBio ? std::min(static_cast<int>(_maxSendPacketSize), static_cast<int>(buf.b.end() - buf.i))
                             : static_cast<int>(buf.b.end() - buf.i);

    while (buf.i != buf.b.end())
    {
        ERR_clear_error(); // Clear any spurious errors.
        int ret;
        if (_memBio)
        {
            if (_sentBytes)
            {
                ret = _sentBytes;
                _sentBytes = 0;
            }
            else
            {
                ret = SSL_write(_ssl, reinterpret_cast<const void*>(&*buf.i), packetSize);
                if (ret > 0)
                {
                    if (!send())
                    {
                        _sentBytes = ret;
                        return IceInternal::SocketOperationWrite;
                    }
                }
            }
        }
        else
        {
            ret = SSL_write(_ssl, reinterpret_cast<const void*>(&*buf.i), packetSize);
        }

        if (ret <= 0)
        {
            switch (SSL_get_error(_ssl, ret))
            {
                case SSL_ERROR_NONE:
                    assert(false);
                    break;
                case SSL_ERROR_ZERO_RETURN:
                {
                    throw ConnectionLostException(__FILE__, __LINE__, IceInternal::getSocketErrno());
                }
                case SSL_ERROR_WANT_READ:
                {
                    assert(false);
                    break;
                }
                case SSL_ERROR_WANT_WRITE:
                {
                    if (_memBio && send())
                    {
                        continue;
                    }
                    return IceInternal::SocketOperationWrite;
                }
                case SSL_ERROR_SYSCALL:
                {
                    if (!_memBio)
                    {
                        if (IceInternal::interrupted())
                        {
                            continue;
                        }

                        if (IceInternal::noBuffers() && packetSize > 1024)
                        {
                            packetSize /= 2;
                            continue;
                        }

                        if (IceInternal::wouldBlock())
                        {
                            assert(SSL_want_write(_ssl));
                            return IceInternal::SocketOperationWrite;
                        }
                    }

                    if (IceInternal::connectionLost() || IceInternal::getSocketErrno() == 0)
                    {
                        throw ConnectionLostException(__FILE__, __LINE__, IceInternal::getSocketErrno());
                    }
                    else
                    {
                        throw SocketException(__FILE__, __LINE__, IceInternal::getSocketErrno());
                    }
                }
                case SSL_ERROR_SSL:
                {
                    throw ProtocolException(
                        __FILE__,
                        __LINE__,
                        "SSL protocol error during write:\n" + _engine->sslErrors());
                }
            }
        }

        buf.i += ret;

        if (packetSize > buf.b.end() - buf.i)
        {
            packetSize = static_cast<int>(buf.b.end() - buf.i);
        }
    }
    return IceInternal::SocketOperationNone;
}

IceInternal::SocketOperation
OpenSSL::TransceiverI::read(IceInternal::Buffer& buf)
{
    if (!_connected)
    {
        return _delegate->read(buf);
    }

    if (_memBio && _readBuffer.i != _readBuffer.b.end())
    {
        if (!receive())
        {
            return IceInternal::SocketOperationRead;
        }
    }

    //
    // Note: We assume that OpenSSL doesn't read more SSL records than
    // necessary to fill the requested data and that the sender sends
    // Ice messages in individual SSL records.
    //

    if (buf.i == buf.b.end())
    {
        return IceInternal::SocketOperationNone;
    }

    _delegate->getNativeInfo()->ready(IceInternal::SocketOperationRead, false);

    //
    // It's impossible for packetSize to be more than an Int.
    //
    int packetSize = static_cast<int>(buf.b.end() - buf.i);
    while (buf.i != buf.b.end())
    {
        ERR_clear_error(); // Clear any spurious errors.
        int ret = SSL_read(_ssl, reinterpret_cast<void*>(&*buf.i), packetSize);
        if (ret <= 0)
        {
            switch (SSL_get_error(_ssl, ret))
            {
                case SSL_ERROR_NONE:
                {
                    assert(false);
                    break;
                }
                case SSL_ERROR_ZERO_RETURN:
                {
                    throw ConnectionLostException(__FILE__, __LINE__, 0);
                }
                case SSL_ERROR_WANT_READ:
                {
                    if (_memBio && receive())
                    {
                        continue;
                    }
                    return IceInternal::SocketOperationRead;
                }
                case SSL_ERROR_WANT_WRITE:
                {
                    assert(false);
                    break;
                }
                case SSL_ERROR_SYSCALL:
                {
                    if (!_memBio)
                    {
                        if (IceInternal::interrupted())
                        {
                            continue;
                        }

                        if (IceInternal::noBuffers() && packetSize > 1024)
                        {
                            packetSize /= 2;
                            continue;
                        }

                        if (IceInternal::wouldBlock())
                        {
                            assert(SSL_want_read(_ssl));
                            return IceInternal::SocketOperationRead;
                        }
                    }

                    if (IceInternal::connectionLost() || IceInternal::getSocketErrno() == 0)
                    {
                        throw ConnectionLostException(__FILE__, __LINE__, IceInternal::getSocketErrno());
                    }
                    else
                    {
                        throw SocketException(__FILE__, __LINE__, IceInternal::getSocketErrno());
                    }
                }
                case SSL_ERROR_SSL:
                {
#if defined(SSL_R_UNEXPECTED_EOF_WHILE_READING)
                    if (SSL_R_UNEXPECTED_EOF_WHILE_READING == ERR_GET_REASON(ERR_peek_error()))
                    {
                        ERR_clear_error();
                        throw ConnectionLostException(__FILE__, __LINE__, 0);
                    }
                    else
                    {
#endif
                        throw ProtocolException(
                            __FILE__,
                            __LINE__,
                            "SSL protocol error during read:\n" + _engine->sslErrors());
#if defined(SSL_R_UNEXPECTED_EOF_WHILE_READING)
                    }
#endif
                }
            }
        }

        buf.i += ret;

        if (packetSize > buf.b.end() - buf.i)
        {
            packetSize = static_cast<int>(buf.b.end() - buf.i);
        }
    }

    //
    // Check if there's still buffered data to read, set the read ready status.
    //
    _delegate->getNativeInfo()->ready(IceInternal::SocketOperationRead, SSL_pending(_ssl) > 0);

    return IceInternal::SocketOperationNone;
}

bool
OpenSSL::TransceiverI::isWaitingToBeRead() const noexcept
{
    return _delegate->isWaitingToBeRead();
}

string
OpenSSL::TransceiverI::protocol() const
{
    return _instance->protocol();
}

string
OpenSSL::TransceiverI::toString() const
{
    return _delegate->toString();
}

string
OpenSSL::TransceiverI::toDetailedString() const
{
    return toString();
}

Ice::ConnectionInfoPtr
OpenSSL::TransceiverI::getInfo() const
{
    auto info = std::make_shared<ConnectionInfo>();
    info->underlying = _delegate->getInfo();
    info->incoming = _incoming;
    info->adapterName = _adapterName;
    info->certs = _certs;
    return info;
}

void
OpenSSL::TransceiverI::checkSendSize(const IceInternal::Buffer&)
{
}

void
OpenSSL::TransceiverI::setBufferSize(int rcvSize, int sndSize)
{
    _delegate->setBufferSize(rcvSize, sndSize);
}

int
OpenSSL::TransceiverI::verifyCallback(int ok, X509_STORE_CTX* ctx)
{
    assert(_remoteCertificateVerificationCallback);
    try
    {
        bool verified =
            _remoteCertificateVerificationCallback(ok, ctx, dynamic_pointer_cast<ConnectionInfo>(getInfo()));
        if (!verified)
        {
            long result = SSL_get_verify_result(_ssl);
            if (result == X509_V_OK)
            {
                throw SecurityException(
                    __FILE__,
                    __LINE__,
                    "IceSSL: certificate verification failed. the certificate was explicitly rejected by the remote "
                    "certificate validation callback.");
            }
            else
            {
                ostringstream os;
                os << "IceSSL: certificate verification failed:\n" << X509_verify_cert_error_string(result);
                const string msg = os.str();
                if (_engine->securityTraceLevel() >= 1)
                {
                    _instance->logger()->trace(_instance->traceCategory(), msg);
                }
                throw SecurityException(__FILE__, __LINE__, msg);
            }
        }
    }
    catch (...)
    {
        _verificationException = current_exception();
    }
    // Allow the SSL handshake to continue, the transceiver initialization will throw an exception if the verification
    // fails.
    return 1;
}

OpenSSL::TransceiverI::TransceiverI(
    const InstancePtr& instance,
    const IceInternal::TransceiverPtr& delegate,
    const string& adapterName,
    const ServerAuthenticationOptions& serverAuthenticationOptions)
    : _instance(instance),
      _engine(dynamic_pointer_cast<OpenSSL::SSLEngine>(instance->engine())),
      _host(""),
      _adapterName(adapterName),
      _incoming(true),
      _delegate(delegate),
      _connected(false),
      _ssl(nullptr),
      _sslCtx(nullptr),
      _memBio(nullptr),
      _sentBytes(0),
      _maxSendPacketSize(0),
      _maxRecvPacketSize(0),
      _localSSLContextSelectionCallback(serverAuthenticationOptions.serverSSLContextSelectionCallback),
      _remoteCertificateVerificationCallback(
          serverAuthenticationOptions.clientCertificateValidationCallback
              ? serverAuthenticationOptions.clientCertificateValidationCallback
              : createDefaultVerificationCallback()),
      _sslNewSessionCallback(serverAuthenticationOptions.sslNewSessionCallback)
{
}

OpenSSL::TransceiverI::TransceiverI(
    const InstancePtr& instance,
    const IceInternal::TransceiverPtr& delegate,
    const string& host,
    const ClientAuthenticationOptions& clientAuthenticationOptions)
    : _instance(instance),
      _engine(dynamic_pointer_cast<OpenSSL::SSLEngine>(instance->engine())),
      _host(host),
      _adapterName(""),
      _incoming(false),
      _delegate(delegate),
      _connected(false),
      _ssl(nullptr),
      _sslCtx(nullptr),
      _memBio(nullptr),
      _sentBytes(0),
      _maxSendPacketSize(0),
      _maxRecvPacketSize(0),
      _localSSLContextSelectionCallback(clientAuthenticationOptions.clientSSLContextSelectionCallback),
      _remoteCertificateVerificationCallback(
          clientAuthenticationOptions.serverCertificateValidationCallback
              ? clientAuthenticationOptions.serverCertificateValidationCallback
              : createDefaultVerificationCallback()),
      _sslNewSessionCallback(clientAuthenticationOptions.sslNewSessionCallback)
{
}

OpenSSL::TransceiverI::~TransceiverI() {}

bool
OpenSSL::TransceiverI::receive()
{
    if (_readBuffer.i == _readBuffer.b.end())
    {
        assert(BIO_ctrl_get_read_request(_memBio));
        _readBuffer.b.resize(BIO_ctrl_get_read_request(_memBio));
        _readBuffer.i = _readBuffer.b.begin();
    }

    while (_readBuffer.i != _readBuffer.b.end())
    {
        if (_delegate->read(_readBuffer) != IceInternal::SocketOperationNone)
        {
            return false;
        }
    }

    assert(_readBuffer.i == _readBuffer.b.end());

#ifndef NDEBUG
    int n =
#endif
        BIO_write(_memBio, &_readBuffer.b[0], static_cast<int>(_readBuffer.b.end() - _readBuffer.b.begin()));

    assert(n == static_cast<int>(_readBuffer.b.end() - _readBuffer.b.begin()));

    return true;
}

bool
OpenSSL::TransceiverI::send()
{
    if (_writeBuffer.i == _writeBuffer.b.end())
    {
        assert(BIO_ctrl_pending(_memBio));
        _writeBuffer.b.resize(BIO_ctrl_pending(_memBio));
        _writeBuffer.i = _writeBuffer.b.begin();
        BIO_read(_memBio, _writeBuffer.i, static_cast<int>(_writeBuffer.b.size()));
    }

    if (_writeBuffer.i != _writeBuffer.b.end())
    {
        if (_delegate->write(_writeBuffer) != IceInternal::SocketOperationNone)
        {
            return false;
        }
    }
    return _writeBuffer.i == _writeBuffer.b.end();
}
