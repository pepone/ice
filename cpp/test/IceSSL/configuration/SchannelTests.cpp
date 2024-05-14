//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

#include "../../src/Ice/SSL/SecureTransportUtil.h"
#include "Ice/SSL/ClientAuthenticationOptions.h"
#include "Ice/SSL/ServerAuthenticationOptions.h"
#include "Test.h"
#include "TestHelper.h"
#include "TestI.h"

#include <algorithm>
#include <fstream>
#include <memory>
#include <optional>
#include <string>
#include <vector>

using namespace std;
using namespace Ice;
using namespace Ice::SSL;
using namespace Test;

#ifdef ICE_USE_SCHANNEL

const string password = "password";

vector<unsigned char>
readFile(const string& path)
{
    ifstream file(path, ios::binary);
    if (!file)
    {
        throw runtime_error("Unable to read certificate file");
    }
    return vector<unsigned char>((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
}

HCERTSTORE
loadPfxStore(const string& path)
{
    vector<unsigned char> buffer = readFile(path);
    if (buffer.empty())
    {
        throw runtime_error("The PFX file buffer is empty.");
    }

    CRYPT_DATA_BLOB pfxBlob;
    pfxBlob.cbData = static_cast<DWORD>(buffer.size());
    pfxBlob.pbData = reinterpret_cast<BYTE*>(&buffer[0]);

    HCERTSTORE store = PFXImportCertStore(&pfxBlob, L"password", CRYPT_USER_KEYSET);
    if (store == nullptr)
    {
        throw runtime_error("Unable to import certificate");
    }
    return store;
}

PCCERT_CONTEXT
loadCertificateContext(const string& certificatePath)
{
    HCERTSTORE store = loadPfxStore(certificatePath);
    PCCERT_CONTEXT cert = CertFindCertificateInStore(store, X509_ASN_ENCODING, 0, CERT_FIND_ANY, 0, nullptr);
    CertCloseStore(store, 0);
    if (!cert)
    {
        throw runtime_error("Unable to import certificate");
    }
    return cert;
}

HCERTSTORE
loadTrustedRootCertificates(const string& path)
{
    HCERTSTORE store = CertOpenStore(CERT_STORE_PROV_MEMORY, 0, 0, 0, nullptr);
    if (!store)
    {
        throw runtime_error("Unable to open memory store");
    }

    vector<unsigned char> buffer = readFile(path);
    if (buffer.empty())
    {
        CertCloseStore(store, 0);
        throw runtime_error("CA certificate file is empty");
    }

    if (!CertAddEncodedCertificateToStore(
            store,
            X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
            &buffer[0],
            static_cast<DWORD>(buffer.size()),
            CERT_STORE_ADD_NEW,
            nullptr))
    {
        CertCloseStore(store, 0);
        throw runtime_error("Error adding certificate to store");
    }
    return store;
}

Ice::CommunicatorPtr
createServer(ServerAuthenticationOptions serverAuthenticationOptions, TestHelper* helper)
{
    Ice::CommunicatorPtr communicator = initialize();
    ObjectAdapterPtr adapter = communicator->createObjectAdapterWithEndpoints(
        "ServerAdapter",
        helper->getTestEndpoint(10, "ssl"),
        serverAuthenticationOptions);
    adapter->add(make_shared<ServerI>(communicator), Identity{.name = "server"});
    adapter->activate();
    return communicator;
}

Ice::CommunicatorPtr
createClient(optional<ClientAuthenticationOptions> clientAuthenticationOptions = nullopt)
{
    return initialize(Ice::InitializationData{.clientAuthenticationOptions = clientAuthenticationOptions});
}

void
clientValidatesServerSettingTrustedRootCertificates(Test::TestHelper* helper, const string& certificatesPath)
{
    cout << "client validates server certificate setting trusted root certificates... " << flush;
    PCCERT_CONTEXT serverCertificate = loadCertificateContext(certificatesPath + "/s_rsa_ca1.p12");
    HCERTSTORE trustedRootCertificates = loadTrustedRootCertificates(certificatesPath + "/cacert1.der");

    try
    {
        Ice::SSL::ServerAuthenticationOptions serverAuthenticationOptions{
            .serverCertificateSelectionCallback = [serverCertificate](const string&)
            {
                CertDuplicateCertificateContext(serverCertificate);
                return serverCertificate;
            }};
        Ice::CommunicatorHolder serverCommunicator(createServer(serverAuthenticationOptions, helper));

        Ice::SSL::ClientAuthenticationOptions ClientAuthenticationOptions{
            .trustedRootCertificates = trustedRootCertificates};
        Ice::CommunicatorHolder clientCommunicator(createClient(ClientAuthenticationOptions));

        ServerPrx obj(clientCommunicator.communicator(), "server:" + helper->getTestEndpoint(10, "ssl"));
        obj->ice_ping();
    }
    catch (...)
    {
        CertFreeCertificateContext(serverCertificate);
        CertCloseStore(trustedRootCertificates, 0);
        throw;
    }
    CertFreeCertificateContext(serverCertificate);
    CertCloseStore(trustedRootCertificates, 0);
    cout << "ok" << endl;
}

void
clientRejectsServerSettingTrustedRootCertificates(Test::TestHelper* helper, const string& certificatesPath)
{
    cout << "client rejects server certificate setting trusted root certificates... " << flush;
    PCCERT_CONTEXT serverCertificate = loadCertificateContext(certificatesPath + "/s_rsa_ca1.p12");

    // The client trusted roots don't include the server certificate CA.
    HCERTSTORE trustedRootCertificates = loadTrustedRootCertificates(certificatesPath + "/cacert2.der");
    try
    {
        Ice::SSL::ServerAuthenticationOptions serverAuthenticationOptions{
            .serverCertificateSelectionCallback = [serverCertificate](const string&)
            {
                CertDuplicateCertificateContext(serverCertificate);
                return serverCertificate;
            }};
        Ice::CommunicatorHolder serverCommunicator(createServer(serverAuthenticationOptions, helper));

        Ice::SSL::ClientAuthenticationOptions clientAuthenticationOptions{
            .trustedRootCertificates = trustedRootCertificates};
        Ice::CommunicatorHolder clientCommunicator(createClient(clientAuthenticationOptions));

        ServerPrx obj(clientCommunicator.communicator(), "server:" + helper->getTestEndpoint(10, "ssl"));
        try
        {
            obj->ice_ping();
            test(false);
        }
        catch (const Ice::SecurityException&)
        {
            // Expected
        }
    }
    catch (...)
    {
        CertFreeCertificateContext(serverCertificate);
        CertCloseStore(trustedRootCertificates, 0);
        throw;
    }
    CertFreeCertificateContext(serverCertificate);
    CertCloseStore(trustedRootCertificates, 0);
    cout << "ok" << endl;
}

void
clientRejectsServerUsingDefaultTrustedRootCertificates(Test::TestHelper* helper, const string& certificatesPath)
{
    cout << "client rejects server certificate using default trusted root certificates... " << flush;
    PCCERT_CONTEXT serverCertificate = loadCertificateContext(certificatesPath + "/s_rsa_ca1.p12");
    try
    {
        Ice::SSL::ServerAuthenticationOptions serverAuthenticationOptions{
            .serverCertificateSelectionCallback = [serverCertificate](const string&)
            {
                CertDuplicateCertificateContext(serverCertificate);
                return serverCertificate;
            }};
        Ice::CommunicatorHolder serverCommunicator(createServer(serverAuthenticationOptions, helper));

        // The client doesn't set trusted root certificates, it would use the system trusted root certificates.
        Ice::CommunicatorHolder clientCommunicator(createClient());

        ServerPrx obj(clientCommunicator.communicator(), "server:" + helper->getTestEndpoint(10, "ssl"));
        try
        {
            obj->ice_ping();
            test(false);
        }
        catch (const Ice::SecurityException&)
        {
            // Expected
        }
    }
    catch (...)
    {
        CertFreeCertificateContext(serverCertificate);
        throw;
    }
    CertFreeCertificateContext(serverCertificate);
    cout << "ok" << endl;
}

void
clientRejectsServerUsingValidationCallback(Test::TestHelper* helper, const string& certificatesPath)
{
    cout << "client rejects server certificate using validation callback... " << flush;
    PCCERT_CONTEXT serverCertificate = loadCertificateContext(certificatesPath + "/s_rsa_ca1.p12");

    // The client trusted root certificates include the server certificate CA, but the validation callback
    // rejects the server certificate.
    HCERTSTORE trustedRootCertificates = loadTrustedRootCertificates(certificatesPath + "/cacert1.der");
    try
    {
        Ice::SSL::ServerAuthenticationOptions serverAuthenticationOptions{
            .serverCertificateSelectionCallback = [serverCertificate](const string&)
            {
                CertDuplicateCertificateContext(serverCertificate);
                return serverCertificate;
            }};
        Ice::CommunicatorHolder serverCommunicator(createServer(serverAuthenticationOptions, helper));

        Ice::SSL::ClientAuthenticationOptions clientAuthenticationOptions{
            .trustedRootCertificates = trustedRootCertificates,
            .serverCertificateValidationCallback = [](CtxtHandle, const Ice::SSL::ConnectionInfoPtr&)
            { return false; }};
        Ice::CommunicatorHolder clientCommunicator(createClient(clientAuthenticationOptions));

        ServerPrx obj(clientCommunicator.communicator(), "server:" + helper->getTestEndpoint(10, "ssl"));
        try
        {
            obj->ice_ping();
            test(false);
        }
        catch (const Ice::SecurityException&)
        {
            // Expected
        }
    }
    catch (...)
    {
        CertFreeCertificateContext(serverCertificate);
        CertCloseStore(trustedRootCertificates, 0);
        throw;
    }
    CertFreeCertificateContext(serverCertificate);
    CertCloseStore(trustedRootCertificates, 0);
    cout << "ok" << endl;
}

void
serverValidatesClientSettingTrustedRootCertificates(Test::TestHelper* helper, const string& certificatesPath)
{
    cout << "server validates client certificate setting trusted root certificates... " << flush;
    PCCERT_CONTEXT serverCertificate = loadCertificateContext(certificatesPath + "/s_rsa_ca1.p12");
    PCCERT_CONTEXT clientCertificate = loadCertificateContext(certificatesPath + "/c_rsa_ca1.p12");
    HCERTSTORE trustedRootCertificates = loadTrustedRootCertificates(certificatesPath + "/cacert1.der");
    try
    {
        Ice::SSL::ServerAuthenticationOptions serverAuthenticationOptions{
            .serverCertificateSelectionCallback =
                [serverCertificate](const string&)
            {
                CertDuplicateCertificateContext(serverCertificate);
                return serverCertificate;
            },
            .trustedRootCertificates = trustedRootCertificates};
        Ice::CommunicatorHolder serverCommunicator(createServer(serverAuthenticationOptions, helper));

        Ice::SSL::ClientAuthenticationOptions clientAuthenticationOptions{
            .clientCertificateSelectionCallback =
                [clientCertificate](const string&)
            {
                CertDuplicateCertificateContext(clientCertificate);
                return clientCertificate;
            },
            .trustedRootCertificates = trustedRootCertificates};
        Ice::CommunicatorHolder clientCommunicator(createClient(clientAuthenticationOptions));

        ServerPrx obj(clientCommunicator.communicator(), "server:" + helper->getTestEndpoint(10, "ssl"));
        obj->ice_ping();
    }
    catch (...)
    {
        CertFreeCertificateContext(clientCertificate);
        CertFreeCertificateContext(serverCertificate);
        CertCloseStore(trustedRootCertificates, 0);
        throw;
    }
    CertFreeCertificateContext(clientCertificate);
    CertFreeCertificateContext(serverCertificate);
    CertCloseStore(trustedRootCertificates, 0);
    cout << "ok" << endl;
}

void
serverRejectsClientSettingTrustedRootCertificates(Test::TestHelper* helper, const string& certificatesPath)
{
    cout << "server rejects client certificate setting trusted root certificates... " << flush;

    PCCERT_CONTEXT serverCertificate = loadCertificateContext(certificatesPath + "/s_rsa_ca1.p12");
    // The server root certificates don't include the client certificate CA.
    HCERTSTORE serverRootCertificates = loadTrustedRootCertificates(certificatesPath + "/cacert2.der");

    PCCERT_CONTEXT clientCertificate = loadCertificateContext(certificatesPath + "/c_rsa_ca1.p12");
    HCERTSTORE clientRootCertificates = loadTrustedRootCertificates(certificatesPath + "/cacert1.der");
    try
    {
        Ice::SSL::ServerAuthenticationOptions serverAuthenticationOptions{
            .serverCertificateSelectionCallback =
                [serverCertificate](const string&)
            {
                CertDuplicateCertificateContext(serverCertificate);
                return serverCertificate;
            },
            .clientCertificateRequired = true,
            // The server CA doesn't trust the client certificate.
            .trustedRootCertificates = serverRootCertificates};
        Ice::CommunicatorHolder serverCommunicator(createServer(serverAuthenticationOptions, helper));

        Ice::CommunicatorHolder clientCommunicator(initialize(Ice::InitializationData{
            .clientAuthenticationOptions = Ice::SSL::ClientAuthenticationOptions{
                .clientCertificateSelectionCallback =
                    [clientCertificate](const string&)
                {
                    CertDuplicateCertificateContext(clientCertificate);
                    return clientCertificate;
                },
                .trustedRootCertificates = clientRootCertificates}}));

        ServerPrx obj(clientCommunicator.communicator(), "server:" + helper->getTestEndpoint(10, "ssl"));
        try
        {
            obj->ice_ping();
            test(false);
        }
        catch (const Ice::ConnectionLostException&)
        {
            // Expected
        }
    }
    catch (...)
    {
        CertFreeCertificateContext(serverCertificate);
        CertCloseStore(serverRootCertificates, 0);

        CertFreeCertificateContext(clientCertificate);
        CertCloseStore(clientRootCertificates, 0);
        throw;
    }
    CertFreeCertificateContext(serverCertificate);
    CertCloseStore(serverRootCertificates, 0);

    CertFreeCertificateContext(clientCertificate);
    CertCloseStore(clientRootCertificates, 0);
    cout << "ok" << endl;
}

void
serverRejectsClientUsingDefaultTrustedRootCertificates(Test::TestHelper* helper, const string& certificatesPath)
{
    cout << "server rejects client certificate using default trusted root certificates... " << flush;
    PCCERT_CONTEXT serverCertificate = loadCertificateContext(certificatesPath + "/s_rsa_ca1.p12");
    PCCERT_CONTEXT clientCertificate = loadCertificateContext(certificatesPath + "/c_rsa_ca1.p12");
    HCERTSTORE trustedRootCertificates = loadTrustedRootCertificates(certificatesPath + "/cacert1.der");
    try
    {
        // No trusted root certificates are set on the server, it would use the system trusted root
        // certificates.
        Ice::SSL::ServerAuthenticationOptions serverAuthenticationOptions{
            .serverCertificateSelectionCallback =
                [serverCertificate](const string&)
            {
                CertDuplicateCertificateContext(serverCertificate);
                return serverCertificate;
            },
            .clientCertificateRequired = true,
        };
        Ice::CommunicatorHolder serverCommunicator(createServer(serverAuthenticationOptions, helper));

        Ice::CommunicatorHolder clientCommunicator(initialize(Ice::InitializationData{
            .clientAuthenticationOptions = Ice::SSL::ClientAuthenticationOptions{
                .clientCertificateSelectionCallback =
                    [clientCertificate](const string&)
                {
                    CertDuplicateCertificateContext(clientCertificate);
                    return clientCertificate;
                },
                .trustedRootCertificates = trustedRootCertificates}}));

        ServerPrx obj(clientCommunicator.communicator(), "server:" + helper->getTestEndpoint(10, "ssl"));
        try
        {
            obj->ice_ping();
            test(false);
        }
        catch (const Ice::ConnectionLostException&)
        {
            // Expected
        }
    }
    catch (...)
    {
        CertFreeCertificateContext(serverCertificate);
        CertFreeCertificateContext(clientCertificate);
        CertCloseStore(trustedRootCertificates, 0);
        throw;
    }
    CertFreeCertificateContext(serverCertificate);
    CertFreeCertificateContext(clientCertificate);
    CertCloseStore(trustedRootCertificates, 0);
    cout << "ok" << endl;
}

void
serverRejectsClientUsingValidationCallback(Test::TestHelper* helper, const string& certificatesPath)
{
    cout << "server rejects client certificate using validation callback... " << flush;
    PCCERT_CONTEXT serverCertificate = loadCertificateContext(certificatesPath + "/s_rsa_ca1.p12");
    PCCERT_CONTEXT clientCertificate = loadCertificateContext(certificatesPath + "/c_rsa_ca1.p12");
    HCERTSTORE trustedRootCertificates = loadTrustedRootCertificates(certificatesPath + "/cacert1.der");
    try
    {
        // The server configured trusted root certificates, trust the client certificate, but the validation
        // callback rejects the client certificate.
        Ice::SSL::ServerAuthenticationOptions serverAuthenticationOptions{
            .serverCertificateSelectionCallback =
                [serverCertificate](const string&)
            {
                CertDuplicateCertificateContext(serverCertificate);
                return serverCertificate;
            },
            .clientCertificateRequired = true,
            .trustedRootCertificates = trustedRootCertificates,
            .clientCertificateValidationCallback = [](CtxtHandle, const Ice::SSL::ConnectionInfoPtr&)
            { return false; }};
        Ice::CommunicatorHolder serverCommunicator(createServer(serverAuthenticationOptions, helper));

        Ice::CommunicatorHolder clientCommunicator(initialize(Ice::InitializationData{
            .clientAuthenticationOptions = Ice::SSL::ClientAuthenticationOptions{
                .clientCertificateSelectionCallback =
                    [clientCertificate](const string&)
                {
                    CertDuplicateCertificateContext(clientCertificate);
                    return clientCertificate;
                },
                .trustedRootCertificates = trustedRootCertificates}}));

        ServerPrx obj(clientCommunicator.communicator(), "server:" + helper->getTestEndpoint(10, "ssl"));
        try
        {
            obj->ice_ping();
            test(false);
        }
        catch (const Ice::ConnectionLostException&)
        {
            // Expected
        }
    }
    catch (...)
    {
        CertFreeCertificateContext(serverCertificate);
        CertFreeCertificateContext(clientCertificate);
        CertCloseStore(trustedRootCertificates, 0);
        throw;
    }
    CertFreeCertificateContext(serverCertificate);
    CertFreeCertificateContext(clientCertificate);
    CertCloseStore(trustedRootCertificates, 0);
    cout << "ok" << endl;
}

void
allSchannelTests(Test::TestHelper* helper, const string& testDir)
{
    const string certificatesPath = testDir + "/../certs";

    clientValidatesServerSettingTrustedRootCertificates(helper, certificatesPath);
    clientRejectsServerSettingTrustedRootCertificates(helper, certificatesPath);
    clientRejectsServerUsingDefaultTrustedRootCertificates(helper, certificatesPath);
    clientRejectsServerUsingValidationCallback(helper, certificatesPath);

    serverValidatesClientSettingTrustedRootCertificates(helper, certificatesPath);
    serverRejectsClientSettingTrustedRootCertificates(helper, certificatesPath);
    serverRejectsClientUsingDefaultTrustedRootCertificates(helper, certificatesPath);
    serverRejectsClientUsingValidationCallback(helper, certificatesPath);
}
#endif
