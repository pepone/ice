//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

//
// NOTE: This test is not interoperable with other language mappings.
//

using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Net;
using System.Security.Cryptography;
using System.Security.Cryptography.X509Certificates;
using System.Threading;
using Test;
using ZeroC.Ice;

namespace ZeroC.IceSSL.Test.Configuration
{
    public class AllTests
    {
        private static bool IsCatalinaOrGreater =>
            AssemblyUtil.IsMacOS && Environment.OSVersion.Version.Major >= 19;

        private static X509Certificate2 createCertificate(string certPEM) =>
            new X509Certificate2(System.Text.Encoding.ASCII.GetBytes(certPEM));

        private static Dictionary<string, string>
        CreateProperties(Dictionary<string, string> defaultProperties, string? cert = null, string? ca = null)
        {
            var properties = new Dictionary<string, string>(defaultProperties);
            string? value;
            if (defaultProperties.TryGetValue("IceSSL.DefaultDir", out value))
            {
                properties["IceSSL.DefaultDir"] = value;
            }

            if (defaultProperties.TryGetValue("Ice.Default.Host", out value))
            {
                properties["Ice.Default.Host"] = value;
            }

            if (defaultProperties.TryGetValue("Ice.IPv6", out value))
            {
                properties["Ice.IPv6"] = value;
            }

            properties["Ice.RetryIntervals"] = "-1";
            //properties["IceSSL.Trace.Security"] = "1";

            if (cert != null)
            {
                properties["IceSSL.CertFile"] = $"{cert}.p12";
            }

            if (ca != null)
            {
                properties["IceSSL.CAs"] = $"{ca}.pem";
            }
            properties["IceSSL.Password"] = "password";

            return properties;
        }

        public static IServerFactoryPrx allTests(TestHelper helper, string testDir)
        {
            Communicator? communicator = helper.Communicator();
            TestHelper.Assert(communicator != null);
            string factoryRef = "factory:" + helper.GetTestEndpoint(0, "tcp");

            var factory = IServerFactoryPrx.Parse(factoryRef, communicator);

            string defaultHost = communicator.GetProperty("Ice.Default.Host") ?? "";
            string defaultDir = $"{testDir}/../certs";
            Dictionary<string, string> defaultProperties = communicator.GetProperties();
            defaultProperties["IceSSL.DefaultDir"] = defaultDir;

            //
            // Load the CA certificates. We could use the IceSSL.ImportCert property, but
            // it would be nice to remove the CA certificates when the test finishes, so
            // this test manually installs the certificates in the LocalMachine:AuthRoot
            // store.
            //
            // Note that the client and server are assumed to run on the same machine,
            // so the certificates installed by the client are also available to the
            // server.
            //
            string caCert1File = defaultDir + "/cacert1.pem";
            string caCert2File = defaultDir + "/cacert2.pem";
            X509Certificate2 caCert1 = new X509Certificate2(caCert1File);
            X509Certificate2 caCert2 = new X509Certificate2(caCert2File);

            TestHelper.Assert(Enumerable.SequenceEqual(createCertificate(File.ReadAllText(caCert1File)).RawData,
                caCert1.RawData));
            TestHelper.Assert(Enumerable.SequenceEqual(createCertificate(File.ReadAllText(caCert2File)).RawData,
                caCert2.RawData));

            X509Store store = new X509Store(StoreName.AuthRoot, StoreLocation.LocalMachine);
            bool isAdministrator = false;
            if (AssemblyUtil.IsWindows)
            {
                try
                {
                    store.Open(OpenFlags.ReadWrite);
                    isAdministrator = true;
                }
                catch (CryptographicException)
                {
                    store.Open(OpenFlags.ReadOnly);
                    Console.Out.WriteLine(
                        "warning: some test requires administrator privileges, run as Administrator to run all the tests.");
                }
            }

            Dictionary<string, string> clientProperties;
            Dictionary<string, string> serverProperties;
            try
            {
                string[] args = new string[0];

                Console.Out.Write("testing initialization... ");
                Console.Out.Flush();

                {
                    clientProperties = CreateProperties(defaultProperties, "c_rsa_ca1", "cacert1");
                    var comm = new Communicator(ref args, clientProperties);
                    var fact = IServerFactoryPrx.Parse(factoryRef, comm);
                    serverProperties = CreateProperties(defaultProperties, "s_rsa_ca1", "cacert1");
                    IServerPrx? server = fact.createServer(serverProperties);
                    try
                    {
                        server!.IcePing();
                    }
                    catch (Exception ex)
                    {
                        Console.WriteLine(ex.ToString());
                        TestHelper.Assert(false);
                    }
                    fact.destroyServer(server);
                    comm.Destroy();
                }

                {
                    // Supply our own certificate.
                    clientProperties = CreateProperties(defaultProperties);
                    clientProperties["IceSSL.CAs"] = caCert1File;
                    var comm = new Communicator(ref args, clientProperties,
                        tlsClientOptions: new TlsClientOptions()
                        {
                            ClientCertificates = new X509Certificate2Collection
                            {
                                new X509Certificate2(defaultDir + "/c_rsa_ca1.p12", "password")
                            }
                        });
                    var fact = IServerFactoryPrx.Parse(factoryRef, comm);
                    serverProperties = CreateProperties(defaultProperties, "s_rsa_ca1", "cacert1");
                    IServerPrx? server = fact.createServer(serverProperties);
                    try
                    {
                        server!.IcePing();
                    }
                    catch (Exception ex)
                    {
                        Console.WriteLine(ex.ToString());
                        TestHelper.Assert(false);
                    }
                    fact.destroyServer(server);
                    comm.Destroy();
                }

                {
                    // Supply our own CA certificate.
                    clientProperties = CreateProperties(defaultProperties, "c_rsa_ca1");
                    var comm = new Communicator(ref args, clientProperties,
                        tlsClientOptions: new TlsClientOptions()
                        {
                            ServerCertificateCertificateAuthorities = new X509Certificate2Collection()
                            {
                                new X509Certificate2(defaultDir + "/cacert1.pem")
                            }
                        });

                    var fact = IServerFactoryPrx.Parse(factoryRef, comm);
                    serverProperties = CreateProperties(defaultProperties, "s_rsa_ca1", "cacert1");
                    IServerPrx? server = fact.createServer(serverProperties);
                    try
                    {
                        server!.IcePing();
                    }
                    catch (Exception ex)
                    {
                        Console.WriteLine(ex.ToString());
                        TestHelper.Assert(false);
                    }
                    fact.destroyServer(server);
                    comm.Destroy();
                }
                Console.Out.WriteLine("ok");

                Console.Out.Write("testing certificate verification... ");
                Console.Out.Flush();
                {
                    //
                    // Test IceSSL.VerifyPeer=0. Client does not have a certificate,
                    // but it still verifies the server's.
                    //
                    clientProperties = CreateProperties(defaultProperties, ca: "cacert1");
                    var comm = new Communicator(ref args, clientProperties);
                    var fact = IServerFactoryPrx.Parse(factoryRef, comm);
                    serverProperties = CreateProperties(defaultProperties, "s_rsa_ca1");
                    serverProperties["IceSSL.VerifyPeer"] = "0";
                    var server = fact.createServer(serverProperties);
                    try
                    {
                        server!.noCert();
                    }
                    catch (Exception ex)
                    {
                        Console.WriteLine(ex.ToString());
                        TestHelper.Assert(false);
                    }
                    fact.destroyServer(server);
                    comm.Destroy();

                    //
                    // Test IceSSL.VerifyPeer=1. Client does not have a certificate.
                    //
                    clientProperties = CreateProperties(defaultProperties, ca: "cacert1");
                    comm = new Communicator(ref args, clientProperties);
                    fact = IServerFactoryPrx.Parse(factoryRef, comm);
                    serverProperties = CreateProperties(defaultProperties, "s_rsa_ca1");
                    serverProperties["IceSSL.VerifyPeer"] = "1";
                    server = fact.createServer(serverProperties);
                    try
                    {
                        server!.noCert();
                    }
                    catch (Exception ex)
                    {
                        Console.WriteLine(ex.ToString());
                        TestHelper.Assert(false);
                    }
                    fact.destroyServer(server);

                    //
                    // Test IceSSL.VerifyPeer=2. This should fail because the client
                    // does not supply a certificate.
                    //
                    serverProperties = CreateProperties(defaultProperties, "s_rsa_ca1");
                    serverProperties["IceSSL.VerifyPeer"] = "2";
                    server = fact.createServer(serverProperties);
                    try
                    {
                        server!.IcePing();
                        TestHelper.Assert(false);
                    }
                    catch (ConnectionLostException)
                    {
                        // Expected.
                    }
                    catch (Exception ex)
                    {
                        Console.WriteLine(ex.ToString());
                        TestHelper.Assert(false);
                    }
                    fact.destroyServer(server);

                    comm.Destroy();

                    //
                    // Test IceSSL.VerifyPeer=1. Client has a certificate.
                    //
                    // Provide "cacert1" to the client to verify the server
                    // certificate (without this the client connection wouln't be
                    // able to provide the certificate chain).
                    //
                    clientProperties = CreateProperties(defaultProperties, "c_rsa_ca1", "cacert1");
                    comm = new Communicator(ref args, clientProperties);
                    fact = IServerFactoryPrx.Parse(factoryRef, comm);
                    serverProperties = CreateProperties(defaultProperties, "s_rsa_ca1", "cacert1");
                    serverProperties["IceSSL.VerifyPeer"] = "1";
                    server = fact.createServer(serverProperties);
                    try
                    {
                        X509Certificate2 clientCert =
                            new X509Certificate2(defaultDir + "/c_rsa_ca1.p12", "password");
                        server!.checkCert(clientCert.Subject, clientCert.Issuer);

                        X509Certificate2 serverCert =
                            new X509Certificate2(defaultDir + "/s_rsa_ca1.p12", "password");
                        X509Certificate2 caCert = new X509Certificate2(defaultDir + "/cacert1.pem");

                        var tcpConnection = (TcpConnection)server.GetConnection()!;
                        TestHelper.Assert(tcpConnection.Endpoint.IsSecure);
                        TestHelper.Assert(serverCert.Equals(tcpConnection.RemoteCertificate));
                    }
                    catch (Exception ex)
                    {
                        Console.WriteLine(ex.ToString());
                        TestHelper.Assert(false);
                    }
                    fact.destroyServer(server);

                    //
                    // Test IceSSL.VerifyPeer=2. Client has a certificate.
                    //
                    serverProperties = CreateProperties(defaultProperties, "s_rsa_ca1", "cacert1");
                    serverProperties["IceSSL.VerifyPeer"] = "2";
                    server = fact.createServer(serverProperties);
                    try
                    {
                        X509Certificate2 clientCert = new X509Certificate2(defaultDir + "/c_rsa_ca1.p12", "password");
                        server!.checkCert(clientCert.Subject, clientCert.Issuer);
                    }
                    catch (Exception ex)
                    {
                        Console.WriteLine(ex.ToString());
                        TestHelper.Assert(false);
                    }
                    fact.destroyServer(server);

                    comm.Destroy();

                    //
                    // Test IceSSL.VerifyPeer=1. This should fail because the
                    // client doesn't trust the server's CA.
                    //
                    clientProperties = CreateProperties(defaultProperties);
                    comm = new Communicator(ref args, clientProperties);
                    fact = IServerFactoryPrx.Parse(factoryRef, comm);
                    serverProperties = CreateProperties(defaultProperties, "s_rsa_ca1");
                    serverProperties["IceSSL.VerifyPeer"] = "0";
                    server = fact.createServer(serverProperties);
                    try
                    {
                        server!.IcePing();
                        TestHelper.Assert(false);
                    }
                    catch (TransportException)
                    {
                        // Expected.
                    }
                    catch (Exception ex)
                    {
                        Console.WriteLine(ex.ToString());
                        TestHelper.Assert(false);
                    }
                    fact.destroyServer(server);
                    comm.Destroy();

                    //
                    // Test IceSSL.VerifyPeer=1. This should fail because the
                    // server doesn't trust the client's CA.
                    //
                    clientProperties = CreateProperties(defaultProperties, "c_rsa_ca2");
                    comm = new Communicator(ref args, clientProperties);
                    fact = IServerFactoryPrx.Parse(factoryRef, comm);
                    serverProperties = CreateProperties(defaultProperties, "s_rsa_ca1", "cacert1");
                    serverProperties["IceSSL.VerifyPeer"] = "1";
                    server = fact.createServer(serverProperties);
                    try
                    {
                        server!.IcePing();
                        TestHelper.Assert(false);
                    }
                    catch (TransportException)
                    {
                        // Expected.
                    }
                    catch (Exception ex)
                    {
                        Console.WriteLine(ex.ToString());
                        TestHelper.Assert(false);
                    }
                    fact.destroyServer(server);
                    comm.Destroy();

                    //
                    // This should succeed because the self signed certificate used by the server is
                    // trusted.
                    //
                    clientProperties = CreateProperties(defaultProperties, ca: "cacert2");
                    comm = new Communicator(ref args, clientProperties);
                    fact = IServerFactoryPrx.Parse(factoryRef, comm);
                    serverProperties = CreateProperties(defaultProperties, "cacert2");
                    serverProperties["IceSSL.VerifyPeer"] = "0";
                    server = fact.createServer(serverProperties);
                    try
                    {
                        server!.IcePing();
                    }
                    catch (Exception ex)
                    {
                        Console.WriteLine(ex.ToString());
                        TestHelper.Assert(false);
                    }
                    fact.destroyServer(server);
                    comm.Destroy();

                    //
                    // This should fail because the self signed certificate used by the server is not
                    // trusted.
                    //
                    clientProperties = CreateProperties(defaultProperties);
                    comm = new Communicator(ref args, clientProperties);
                    fact = IServerFactoryPrx.Parse(factoryRef, comm);
                    serverProperties = CreateProperties(defaultProperties, "cacert2");
                    serverProperties["IceSSL.VerifyPeer"] = "0";
                    server = fact.createServer(serverProperties);
                    try
                    {
                        server!.IcePing();
                        TestHelper.Assert(false);
                    }
                    catch (TransportException)
                    {
                        // Expected.
                    }
                    catch (Exception ex)
                    {
                        Console.WriteLine(ex.ToString());
                        TestHelper.Assert(false);
                    }
                    fact.destroyServer(server);
                    comm.Destroy();

                    //
                    // Verify that IceSSL.CheckCertName has no effect in a server.
                    //
                    clientProperties = CreateProperties(defaultProperties, "c_rsa_ca1", "cacert1");
                    comm = new Communicator(ref args, clientProperties);
                    fact = IServerFactoryPrx.Parse(factoryRef, comm);
                    serverProperties = CreateProperties(defaultProperties, "s_rsa_ca1", "cacert1");
                    server = fact.createServer(serverProperties);
                    try
                    {
                        server!.IcePing();
                    }
                    catch (Exception ex)
                    {
                        Console.WriteLine(ex.ToString());
                        TestHelper.Assert(false);
                    }
                    fact.destroyServer(server);
                    comm.Destroy();

                    // Test Hostname verification only when Ice.DefaultHost is 127.0.0.1 as that is the IP address used
                    // in the test certificates.
                    if (defaultHost.Equals("127.0.0.1"))
                    {
                        // Test using localhost as target host.
                        var props = new Dictionary<string, string>(defaultProperties);
                        props["Ice.Default.Host"] = "localhost";

                        // This must succeed, the target host matches the certificate DNS altName.
                        {
                            clientProperties = CreateProperties(defaultProperties, "c_rsa_ca1", "cacert1");
                            comm = new Communicator(ref args, clientProperties);

                            fact = IServerFactoryPrx.Parse(factoryRef, comm);
                            serverProperties = CreateProperties(props, "s_rsa_ca1_cn1", "cacert1");
                            server = fact.createServer(serverProperties);
                            try
                            {
                                server!.IcePing();
                            }
                            catch (Exception ex)
                            {
                                Console.WriteLine(ex.ToString());
                                TestHelper.Assert(false);
                            }
                            fact.destroyServer(server);
                            comm.Destroy();
                        }

                        // This must fail, the target host does not match the certificate DNS altName.
                        {
                            clientProperties = CreateProperties(defaultProperties, "c_rsa_ca1", "cacert1");
                            comm = new Communicator(ref args, clientProperties);

                            fact = IServerFactoryPrx.Parse(factoryRef, comm);
                            serverProperties = CreateProperties(props, "s_rsa_ca1_cn2", "cacert1");
                            server = fact.createServer(serverProperties);
                            try
                            {
                                server!.IcePing();
                                TestHelper.Assert(false);
                            }
                            catch (TransportException)
                            {
                                // Expected
                            }
                            fact.destroyServer(server);
                            comm.Destroy();
                        }

                        // This must succeed, the target host matches the certificate Common Name and the certificate
                        // does not include a DNS altName.
                        {
                            clientProperties = CreateProperties(defaultProperties, "c_rsa_ca1", "cacert1");
                            comm = new Communicator(ref args, clientProperties);

                            fact = IServerFactoryPrx.Parse(factoryRef, comm);
                            serverProperties = CreateProperties(props, "s_rsa_ca1_cn3", "cacert1");
                            server = fact.createServer(serverProperties);
                            try
                            {
                                server!.IcePing();
                            }
                            catch (Exception)
                            {
                                // macOS >= Catalina requires a DNS altName. DNS name as the Common Name is not trusted
                                TestHelper.Assert(IsCatalinaOrGreater);
                            }
                            fact.destroyServer(server);
                            comm.Destroy();
                        }

                        // This must fail, the target host does not match the certificate Common Name and the
                        // certificate does not include a DNS altName.
                        {
                            clientProperties = CreateProperties(defaultProperties, "c_rsa_ca1", "cacert1");
                            comm = new Communicator(ref args, clientProperties);

                            fact = IServerFactoryPrx.Parse(factoryRef, comm);
                            serverProperties = CreateProperties(props, "s_rsa_ca1_cn4", "cacert1");
                            server = fact.createServer(serverProperties);
                            try
                            {
                                server!.IcePing();
                                TestHelper.Assert(false);
                            }
                            catch (TransportException)
                            {
                                // Expected
                            }
                            fact.destroyServer(server);
                            comm.Destroy();
                        }

                        // This must fail, the target host matches the certificate Common Name and the certificate has
                        // a DNS altName that does not matches the target host
                        {
                            clientProperties = CreateProperties(defaultProperties, "c_rsa_ca1", "cacert1");
                            comm = new Communicator(ref args, clientProperties);

                            fact = IServerFactoryPrx.Parse(factoryRef, comm);
                            serverProperties = CreateProperties(props, "s_rsa_ca1_cn5", "cacert1");
                            server = fact.createServer(serverProperties);
                            try
                            {
                                server!.IcePing();
                                TestHelper.Assert(false);
                            }
                            catch (TransportException)
                            {
                                // Expected
                            }
                            fact.destroyServer(server);
                            comm.Destroy();
                        }

                        //
                        // Test using 127.0.0.1 as target host
                        //

                        //
                        // Disabled for compatibility with older Windows
                        // versions.
                        //
                        /* //
                        // Target host matches the certificate IP altName
                        //
                        {
                            clientProperties = CreateProperties(defaultProperties, "c_rsa_ca1", "cacert1");
                            comm = new Communicator(ref args, initData);

                            fact = IServerFactoryPrxHelper.checkedCast(comm.stringToProxy(factoryRef));
                            TestHelper.Assert(fact != null);
                            serverProperties = CreateProperties(defaultProperties, "s_rsa_ca1_cn6", "cacert1");
                            server = fact.createServer(d);
                            try
                            {
                                server.IcePing();
                            }
                            catch(Exception)
                            {
                                TestHelper.Assert(false);
                            }
                            fact.destroyServer(server);
                            comm.Destroy();
                        }
                        //
                        // Target host does not match the certificate IP altName
                        //
                        {
                            clientProperties = CreateProperties(defaultProperties, "c_rsa_ca1", "cacert1");
                            comm = new Communicator(ref args, initData);

                            fact = IServerFactoryPrxHelper.checkedCast(comm.stringToProxy(factoryRef));
                            TestHelper.Assert(fact != null);
                            serverProperties = CreateProperties(defaultProperties, "s_rsa_ca1_cn7", "cacert1");
                            server = fact.createServer(d);
                            try
                            {
                                server.IcePing();
                                TestHelper.Assert(false);
                            }
                            catch(Ice.SecurityException)
                            {
                                // Expected
                            }
                            fact.destroyServer(server);
                            comm.Destroy();
                        }*/
                        //
                        // Target host is an IP addres that matches the CN and the certificate doesn't
                        // include an IP altName.
                        //
                        {
                            clientProperties = CreateProperties(defaultProperties, "c_rsa_ca1", "cacert1");
                            comm = new Communicator(ref args, clientProperties);

                            fact = IServerFactoryPrx.Parse(factoryRef, comm);
                            serverProperties = CreateProperties(defaultProperties, "s_rsa_ca1_cn8", "cacert1");
                            server = fact.createServer(serverProperties);
                            try
                            {
                                server!.IcePing();
                            }
                            catch (TransportException ex)
                            {
                                //
                                // macOS catalina does not check the certificate common name
                                //
                                if (!AssemblyUtil.IsMacOS)
                                {
                                    Console.WriteLine(ex.ToString());
                                    TestHelper.Assert(false);
                                }
                            }
                            fact.destroyServer(server);
                            comm.Destroy();
                        }

                        //
                        // Target host does not match the certificate DNS altName, connection should fail.
                        //
                        {
                            clientProperties = CreateProperties(defaultProperties, "c_rsa_ca1", "cacert1");
                            comm = new Communicator(ref args, clientProperties);

                            fact = IServerFactoryPrx.Parse(factoryRef, comm);
                            serverProperties = CreateProperties(props, "s_rsa_ca1_cn2", "cacert1");
                            server = fact.createServer(serverProperties);
                            try
                            {
                                server!.IcePing();
                                TestHelper.Assert(false);
                            }
                            catch (TransportException)
                            {
                            }
                            catch (Exception ex)
                            {
                                Console.WriteLine(ex.ToString());
                                TestHelper.Assert(false);
                            }
                            fact.destroyServer(server);
                            comm.Destroy();
                        }
                    }
                }
                Console.Out.WriteLine("ok");

                Console.Out.Write("testing certificate selection callback... ");
                Console.Out.Flush();
                {
                    clientProperties = CreateProperties(defaultProperties, null, "cacert1");
                    clientProperties["IceSSL.Password"] = "";

                    bool called = false;
                    var comm = new Communicator(ref args, clientProperties,
                        tlsClientOptions: new TlsClientOptions()
                        {
                            ClientCertificateSelectionCallback =
                                (sender, targetHost, certs, remoteCertificate, acceptableIssuers) =>
                                {
                                    called = true;
                                    return null;
                                }
                        });
                    var fact = IServerFactoryPrx.Parse(factoryRef, comm);
                    serverProperties = CreateProperties(defaultProperties, "s_rsa_ca1", "cacert1");
                    serverProperties["IceSSL.VerifyPeer"] = "0";
                    IServerPrx? server = fact.createServer(serverProperties);
                    TestHelper.Assert(server != null);
                    try
                    {
                        server.noCert();
                        TestHelper.Assert(called);
                    }
                    catch (Exception ex)
                    {
                        Console.WriteLine(ex.ToString());
                        TestHelper.Assert(false);
                    }
                    fact.destroyServer(server);
                    comm.Destroy();

                    var myCerts = new X509Certificate2Collection();
                    myCerts.Import(defaultDir + "/c_rsa_ca1.p12", "password", X509KeyStorageFlags.DefaultKeySet);
                    called = false;
                    comm = new Communicator(ref args, clientProperties,
                        tlsClientOptions: new TlsClientOptions()
                        {
                            ClientCertificateSelectionCallback =
                                (sender, targetHost, certs, remoteCertificate, acceptableIssuers) =>
                                {
                                    called = true;
                                    return myCerts[0];
                                }
                        });

                    serverProperties = CreateProperties(defaultProperties, "s_rsa_ca1", "cacert1");
                    serverProperties["IceSSL.VerifyPeer"] = "2";
                    fact = IServerFactoryPrx.Parse(factoryRef, comm);
                    server = fact.createServer(serverProperties);
                    TestHelper.Assert(server != null);
                    try
                    {
                        server.IcePing();
                        var connection = server.GetCachedConnection() as TcpConnection;
                        TestHelper.Assert(connection != null);
                        TestHelper.Assert(connection.LocalCertificate == myCerts[0]);
                        TestHelper.Assert(called);
                    }
                    catch (Exception ex)
                    {
                        Console.WriteLine(ex.ToString());
                        TestHelper.Assert(false);
                    }
                    fact.destroyServer(server);
                    comm.Destroy();
                }
                Console.Out.WriteLine("ok");

                Console.Out.Write("testing certificate chains... ");
                Console.Out.Flush();
                {
                    X509Store certStore = new X509Store("My", StoreLocation.CurrentUser);
                    certStore.Open(OpenFlags.ReadWrite);
                    X509Certificate2Collection certs = new X509Certificate2Collection();
                    var storageFlags = X509KeyStorageFlags.DefaultKeySet;
                    if (AssemblyUtil.IsMacOS)
                    {
                        //
                        // On macOS, we need to mark the key exportable because the addition of the key to the
                        // cert store requires to move the key from on keychain to another (which requires the
                        // Exportable flag... see https://github.com/dotnet/corefx/issues/25631)
                        //
                        storageFlags |= X509KeyStorageFlags.Exportable;
                    }
                    certs.Import(defaultDir + "/s_rsa_cai2.p12", "password", storageFlags);
                    foreach (X509Certificate2 cert in certs)
                    {
                        certStore.Add(cert);
                    }
                    try
                    {
                        clientProperties = CreateProperties(defaultProperties);
                        Communicator comm = new Communicator(clientProperties);

                        IServerFactoryPrx fact = IServerFactoryPrx.Parse(factoryRef, comm);

                        //
                        // The client can't verify the server certificate it should fail.
                        //
                        serverProperties = CreateProperties(defaultProperties, "s_rsa_ca1");
                        serverProperties["IceSSL.VerifyPeer"] = "0";
                        IServerPrx? server = fact.createServer(serverProperties);
                        try
                        {
                            server!.IcePing();
                            TestHelper.Assert(false);
                        }
                        catch (TransportException)
                        {
                        }
                        catch (Exception ex)
                        {
                            Console.WriteLine(ex.ToString());
                            TestHelper.Assert(false);
                        }
                        fact.destroyServer(server);

                        //
                        // Setting the CA for the server shouldn't change anything, it
                        // shouldn't modify the cert chain sent to the client.
                        //
                        serverProperties = CreateProperties(defaultProperties, "s_rsa_ca1", "cacert1");
                        serverProperties["IceSSL.VerifyPeer"] = "0";
                        server = fact.createServer(serverProperties);
                        try
                        {
                            server!.IcePing();
                            TestHelper.Assert(false);
                        }
                        catch (TransportException)
                        {
                        }
                        catch (Exception ex)
                        {
                            Console.WriteLine(ex.ToString());
                            TestHelper.Assert(false);
                        }
                        fact.destroyServer(server);

                        //
                        // The client can't verify the server certificate should fail.
                        //
                        serverProperties = CreateProperties(defaultProperties, "s_rsa_wroot_ca1");
                        serverProperties["IceSSL.VerifyPeer"] = "0";
                        server = fact.createServer(serverProperties);
                        try
                        {
                            server!.IcePing();
                            TestHelper.Assert(false);
                        }
                        catch (TransportException)
                        {
                        }
                        catch (Exception ex)
                        {
                            Console.WriteLine(ex.ToString());
                            TestHelper.Assert(false);
                        }
                        fact.destroyServer(server);
                        comm.Destroy();

                        //
                        // Now the client verifies the server certificate
                        //
                        clientProperties = CreateProperties(defaultProperties, ca: "cacert1");
                        comm = new Communicator(clientProperties);

                        fact = IServerFactoryPrx.Parse(factoryRef, comm);

                        {
                            serverProperties = CreateProperties(defaultProperties, "s_rsa_ca1");
                            serverProperties["IceSSL.VerifyPeer"] = "0";
                            server = fact.createServer(serverProperties);
                            try
                            {
                                var tcpConnection = (TcpConnection)server!.GetConnection()!;
                                TestHelper.Assert(tcpConnection.Endpoint.IsSecure);
                            }
                            catch (Exception ex)
                            {
                                Console.WriteLine(ex.ToString());
                                TestHelper.Assert(false);
                            }
                            fact.destroyServer(server);
                        }
                    }
                    finally
                    {
                        foreach (X509Certificate2 cert in certs)
                        {
                            certStore.Remove(cert);
                        }
                    }
                }
                Console.Out.WriteLine("ok");

                Console.Out.Write("testing custom certificate verifier... ");
                Console.Out.Flush();
                {
                    // Verify that a server certificate is present.
                    clientProperties = CreateProperties(defaultProperties, "c_rsa_ca1");

                    bool invoked = false;
                    bool hadCert = false;
                    var comm = new Communicator(ref args, clientProperties,
                        tlsClientOptions: new TlsClientOptions()
                        {
                            ServerCertificateValidationCallback = (sender, certificate, chain, sslPolicyErrors) =>
                            {
                                hadCert = certificate != null;
                                invoked = true;
                                return true;
                            }
                        });

                    var fact = IServerFactoryPrx.Parse(factoryRef, comm);
                    serverProperties = CreateProperties(defaultProperties, "s_rsa_ca1", "cacert1");
                    serverProperties["IceSSL.VerifyPeer"] = "2";
                    IServerPrx? server = fact.createServer(serverProperties);
                    try
                    {
                        TestHelper.Assert(server != null);
                        var tcpConnection = (TcpConnection)server.GetConnection()!;
                        TestHelper.Assert(tcpConnection.Endpoint.IsSecure);
                        server.checkCipher(tcpConnection.NegotiatedCipherSuite!.ToString()!);
                    }
                    catch (Exception ex)
                    {
                        Console.WriteLine(ex.ToString());
                        TestHelper.Assert(false);
                    }
                    TestHelper.Assert(invoked);
                    TestHelper.Assert(hadCert);
                    fact.destroyServer(server);
                    comm.Destroy();

                    // Have the verifier return false. Close the connection explicitly to force a new connection to be
                    // established.
                    comm = new Communicator(ref args, clientProperties,
                        tlsClientOptions: new TlsClientOptions()
                        {
                            ServerCertificateValidationCallback = (sender, certificate, chain, sslPolicyErrors) =>
                            {
                                hadCert = certificate != null;
                                invoked = true;
                                return false;
                            }
                        });
                    fact = IServerFactoryPrx.Parse(factoryRef, comm);
                    serverProperties = CreateProperties(defaultProperties, "s_rsa_ca1", "cacert1");
                    serverProperties["IceSSL.VerifyPeer"] = "2";
                    server = fact.createServer(serverProperties);
                    try
                    {
                        TestHelper.Assert(server != null);
                        server.IcePing();
                        TestHelper.Assert(false);
                    }
                    catch (TransportException)
                    {
                        // Expected.
                    }
                    catch (Exception ex)
                    {
                        Console.WriteLine(ex.ToString());
                        TestHelper.Assert(false);
                    }
                    TestHelper.Assert(invoked);
                    TestHelper.Assert(hadCert);
                    fact.destroyServer(server);

                    comm.Destroy();
                }
                Console.Out.WriteLine("ok");

                Console.Out.Write("testing protocols... ");
                Console.Out.Flush();
                {
                    //
                    // This should fail because the client and server have no protocol
                    // in common.
                    //
                    clientProperties = CreateProperties(defaultProperties, "c_rsa_ca1", "cacert1");
                    clientProperties["IceSSL.Protocols"] = "tls1_2";
                    Communicator comm = new Communicator(ref args, clientProperties);
                    var fact = IServerFactoryPrx.Parse(factoryRef, comm);
                    serverProperties = CreateProperties(defaultProperties, "s_rsa_ca1", "cacert1");
                    serverProperties["IceSSL.VerifyPeer"] = "2";
                    serverProperties["IceSSL.Protocols"] = "tls1_3";
                    IServerPrx? server = fact.createServer(serverProperties);
                    try
                    {
                        server!.IcePing();
                        //TestHelper.Assert(false);
                    }
                    catch (TransportException)
                    {
                        // Expected.
                    }
                    catch (Exception ex)
                    {
                        Console.WriteLine(ex.ToString());
                        TestHelper.Assert(false);
                    }
                    fact.destroyServer(server);
                    comm.Destroy();

                    //
                    // This should succeed.
                    //
                    comm = new Communicator(ref args, clientProperties);
                    fact = IServerFactoryPrx.Parse(factoryRef, comm);
                    serverProperties = CreateProperties(defaultProperties, "s_rsa_ca1", "cacert1");
                    serverProperties["IceSSL.VerifyPeer"] = "2";
                    serverProperties["IceSSL.Protocols"] = "tls1_2, tls1_3";
                    server = fact.createServer(serverProperties);

                    server!.IcePing();
                    fact.destroyServer(server);
                    comm.Destroy();

                    try
                    {
                        clientProperties = CreateProperties(defaultProperties, "c_rsa_ca1", "cacert1");
                        clientProperties["IceSSL.Protocols"] = "tls1_2";
                        comm = new Communicator(ref args, clientProperties);
                        fact = IServerFactoryPrx.Parse(factoryRef, comm);
                        serverProperties = CreateProperties(defaultProperties, "s_rsa_ca1", "cacert1");
                        serverProperties["IceSSL.VerifyPeer"] = "2";
                        serverProperties["IceSSL.Protocols"] = "tls1_2";
                        server = fact.createServer(serverProperties);
                        server!.IcePing();

                        fact.destroyServer(server);
                        comm.Destroy();
                    }
                    catch (Exception ex)
                    {
                        Console.WriteLine(ex.ToString());
                        TestHelper.Assert(false);
                    }
                }
                Console.Out.WriteLine("ok");

                Console.Out.Write("testing expired certificates... ");
                Console.Out.Flush();
                {
                    //
                    // This should fail because the server's certificate is expired.
                    //
                    clientProperties = CreateProperties(defaultProperties, "c_rsa_ca1", "cacert1");
                    Communicator comm = new Communicator(ref args, clientProperties);
                    var fact = IServerFactoryPrx.Parse(factoryRef, comm);
                    serverProperties = CreateProperties(defaultProperties, "s_rsa_ca1_exp", "cacert1");
                    serverProperties["IceSSL.VerifyPeer"] = "2";
                    IServerPrx? server = fact.createServer(serverProperties);
                    try
                    {
                        server!.IcePing();
                        TestHelper.Assert(false);
                    }
                    catch (TransportException)
                    {
                        // Expected.
                    }
                    catch (Exception ex)
                    {
                        Console.Out.Write(ex.ToString());
                        TestHelper.Assert(false);
                    }
                    fact.destroyServer(server);
                    comm.Destroy();

                    //
                    // This should fail because the client's certificate is expired.
                    //
                    clientProperties["IceSSL.CertFile"] = "c_rsa_ca1_exp.p12";
                    comm = new Communicator(ref args, clientProperties);
                    fact = IServerFactoryPrx.Parse(factoryRef, comm);
                    serverProperties = CreateProperties(defaultProperties, "s_rsa_ca1", "cacert1");
                    serverProperties["IceSSL.VerifyPeer"] = "2";
                    server = fact.createServer(serverProperties);
                    try
                    {
                        server!.IcePing();
                        TestHelper.Assert(false);
                    }
                    catch (ConnectionLostException)
                    {
                        // Expected.
                    }
                    catch (Exception ex)
                    {
                        Console.Out.Write(ex.ToString());
                        TestHelper.Assert(false);
                    }
                    fact.destroyServer(server);
                    comm.Destroy();
                }
                Console.Out.WriteLine("ok");

                if (AssemblyUtil.IsWindows && isAdministrator)
                {
                    //
                    // LocalMachine certificate store is not supported on non
                    // Windows platforms.
                    //
                    Console.Out.Write("testing multiple CA certificates... ");
                    Console.Out.Flush();
                    {
                        clientProperties = CreateProperties(defaultProperties, "c_rsa_ca1");
                        Communicator comm = new Communicator(ref args, clientProperties);
                        var fact = IServerFactoryPrx.Parse(factoryRef, comm);
                        serverProperties = CreateProperties(defaultProperties, "s_rsa_ca2");
                        serverProperties["IceSSL.VerifyPeer"] = "2";
                        serverProperties["IceSSL.UsePlatformCAs"] = "1";
                        store.Add(caCert1);
                        store.Add(caCert2);
                        IServerPrx? server = fact.createServer(serverProperties);
                        try
                        {
                            server!.IcePing();
                        }
                        catch (Exception ex)
                        {
                            Console.WriteLine(ex.ToString());
                            TestHelper.Assert(false);
                        }
                        fact.destroyServer(server);
                        store.Remove(caCert1);
                        store.Remove(caCert2);
                        comm.Destroy();
                    }
                    Console.Out.WriteLine("ok");
                }

                Console.Out.Write("testing multiple CA certificates... ");
                Console.Out.Flush();
                {
                    clientProperties = CreateProperties(defaultProperties, "c_rsa_ca1", "cacerts");
                    Communicator comm = new Communicator(clientProperties);
                    IServerFactoryPrx fact = IServerFactoryPrx.Parse(factoryRef, comm);
                    serverProperties = CreateProperties(defaultProperties, "s_rsa_ca2", "cacerts");
                    serverProperties["IceSSL.VerifyPeer"] = "2";
                    IServerPrx? server = fact.createServer(serverProperties);
                    try
                    {
                        server!.IcePing();
                    }
                    catch (Exception ex)
                    {
                        Console.WriteLine(ex.ToString());
                        TestHelper.Assert(false);
                    }
                    fact.destroyServer(server);
                    comm.Destroy();
                }
                Console.Out.WriteLine("ok");

                Console.Out.Write("testing DER CA certificate... ");
                Console.Out.Flush();
                {
                    clientProperties = CreateProperties(defaultProperties, "c_rsa_ca1");
                    clientProperties["IceSSL.CAs"] = "cacert1.der";
                    using var comm = new Communicator(clientProperties);
                    var fact = IServerFactoryPrx.Parse(factoryRef, comm);
                    serverProperties = CreateProperties(defaultProperties, "s_rsa_ca1");
                    serverProperties["IceSSL.VerifyPeer"] = "2";
                    serverProperties["IceSSL.CAs"] = "cacert1.der";
                    IServerPrx? server = fact.createServer(serverProperties);
                    try
                    {
                        server!.IcePing();
                    }
                    catch (Exception ex)
                    {
                        Console.WriteLine(ex.ToString());
                        TestHelper.Assert(false);
                    }
                    fact.destroyServer(server);
                }
                Console.Out.WriteLine("ok");

                Console.Out.Write("testing passwords... ");
                Console.Out.Flush();
                {
                    //
                    // Test password failure.
                    //
                    clientProperties = CreateProperties(defaultProperties, "c_rsa_ca1");
                    // Don't specify the password.
                    clientProperties.Remove("IceSSL.Password");
                    try
                    {
                        new Communicator(ref args, clientProperties);
                        TestHelper.Assert(false);
                    }
                    catch (Exception)
                    {
                        // Expected.
                    }
                }

                Console.Out.WriteLine("ok");

                Console.Out.Write("testing IceSSL.TrustOnly... ");
                Console.Out.Flush();
                {
                    clientProperties = CreateProperties(defaultProperties, "c_rsa_ca1", "cacert1");
                    clientProperties["IceSSL.TrustOnly"] =
                        "C=US, ST=Florida, O=ZeroC\\, Inc.,OU=Ice, emailAddress=info@zeroc.com, CN=Server";
                    Communicator comm = new Communicator(ref args, clientProperties);

                    IServerFactoryPrx fact = IServerFactoryPrx.Parse(factoryRef, comm);
                    serverProperties = CreateProperties(defaultProperties, "s_rsa_ca1", "cacert1");
                    IServerPrx? server = fact.createServer(serverProperties);
                    try
                    {
                        server!.IcePing();
                    }
                    catch (Exception ex)
                    {
                        Console.WriteLine(ex.ToString());
                        TestHelper.Assert(false);
                    }
                    fact.destroyServer(server);
                    comm.Destroy();
                }
                {
                    clientProperties = CreateProperties(defaultProperties, "c_rsa_ca1", "cacert1");
                    clientProperties["IceSSL.TrustOnly"] =
                        "!C=US, ST=Florida, O=ZeroC\\, Inc.,OU=Ice, emailAddress=info@zeroc.com, CN=Server";
                    Communicator comm = new Communicator(ref args, clientProperties);

                    IServerFactoryPrx fact = IServerFactoryPrx.Parse(factoryRef, comm);
                    serverProperties = CreateProperties(defaultProperties, "s_rsa_ca1", "cacert1");
                    IServerPrx? server = fact.createServer(serverProperties);
                    try
                    {
                        server!.IcePing();
                        TestHelper.Assert(false);
                    }
                    catch (Exception)
                    {
                    }
                    fact.destroyServer(server);
                    comm.Destroy();
                }
                {
                    clientProperties = CreateProperties(defaultProperties, "c_rsa_ca1", "cacert1");
                    clientProperties["IceSSL.TrustOnly"] =
                        "C=US, ST=Florida, O=\"ZeroC, Inc.\",OU=Ice, emailAddress=info@zeroc.com, CN=Server";
                    Communicator comm = new Communicator(ref args, clientProperties);

                    IServerFactoryPrx fact = IServerFactoryPrx.Parse(factoryRef, comm);
                    serverProperties = CreateProperties(defaultProperties, "s_rsa_ca1", "cacert1");
                    IServerPrx? server = fact.createServer(serverProperties);
                    try
                    {
                        server!.IcePing();
                    }
                    catch (Exception ex)
                    {
                        Console.WriteLine(ex.ToString());
                        TestHelper.Assert(false);
                    }
                    fact.destroyServer(server);
                    comm.Destroy();
                }
                {
                    clientProperties = CreateProperties(defaultProperties, "c_rsa_ca1", "cacert1");
                    Communicator comm = new Communicator(ref args, clientProperties);

                    IServerFactoryPrx fact = IServerFactoryPrx.Parse(factoryRef, comm);
                    serverProperties = CreateProperties(defaultProperties, "s_rsa_ca1", "cacert1");
                    serverProperties["IceSSL.TrustOnly"] =
                        "C=US, ST=Florida, O=ZeroC\\, Inc.,OU=Ice, emailAddress=info@zeroc.com, CN=Client";
                    IServerPrx? server = fact.createServer(serverProperties);
                    try
                    {
                        server!.IcePing();
                    }
                    catch (Exception ex)
                    {
                        Console.WriteLine(ex.ToString());
                        TestHelper.Assert(false);
                    }
                    fact.destroyServer(server);
                    comm.Destroy();
                }
                {
                    clientProperties = CreateProperties(defaultProperties, "c_rsa_ca1", "cacert1");
                    Communicator comm = new Communicator(ref args, clientProperties);

                    var fact = IServerFactoryPrx.Parse(factoryRef, comm);
                    serverProperties = CreateProperties(defaultProperties, "s_rsa_ca1", "cacert1");
                    serverProperties["IceSSL.TrustOnly"] =
                        "!C=US, ST=Florida, O=ZeroC\\, Inc.,OU=Ice, emailAddress=info@zeroc.com, CN=Client";
                    IServerPrx? server = fact.createServer(serverProperties);
                    try
                    {
                        server!.IcePing();
                        TestHelper.Assert(false);
                    }
                    catch (Exception)
                    {
                    }
                    fact.destroyServer(server);
                    comm.Destroy();
                }
                {
                    clientProperties = CreateProperties(defaultProperties, "c_rsa_ca1", "cacert1");
                    clientProperties["IceSSL.TrustOnly"] = "CN=Server";
                    Communicator comm = new Communicator(ref args, clientProperties);

                    var fact = IServerFactoryPrx.Parse(factoryRef, comm);
                    serverProperties = CreateProperties(defaultProperties, "s_rsa_ca1", "cacert1");
                    IServerPrx? server = fact.createServer(serverProperties);
                    try
                    {
                        server!.IcePing();
                    }
                    catch (Exception ex)
                    {
                        Console.WriteLine(ex.ToString());
                        TestHelper.Assert(false);
                    }
                    fact.destroyServer(server);
                    comm.Destroy();
                }
                {
                    clientProperties = CreateProperties(defaultProperties, "c_rsa_ca1", "cacert1");
                    clientProperties["IceSSL.TrustOnly"] = "!CN=Server";
                    Communicator comm = new Communicator(ref args, clientProperties);

                    var fact = IServerFactoryPrx.Parse(factoryRef, comm);
                    serverProperties = CreateProperties(defaultProperties, "s_rsa_ca1", "cacert1");
                    IServerPrx? server = fact.createServer(serverProperties);
                    try
                    {
                        server!.IcePing();
                        TestHelper.Assert(false);
                    }
                    catch (Exception)
                    {
                    }
                    fact.destroyServer(server);
                    comm.Destroy();
                }
                {
                    clientProperties = CreateProperties(defaultProperties, "c_rsa_ca1", "cacert1");
                    Communicator comm = new Communicator(ref args, clientProperties);

                    var fact = IServerFactoryPrx.Parse(factoryRef, comm);
                    serverProperties = CreateProperties(defaultProperties, "s_rsa_ca1", "cacert1");
                    serverProperties["IceSSL.TrustOnly"] = "CN=Client";
                    IServerPrx? server = fact.createServer(serverProperties);
                    try
                    {
                        server!.IcePing();
                    }
                    catch (Exception ex)
                    {
                        Console.WriteLine(ex.ToString());
                        TestHelper.Assert(false);
                    }
                    fact.destroyServer(server);
                    comm.Destroy();
                }
                {
                    clientProperties = CreateProperties(defaultProperties, "c_rsa_ca1", "cacert1");
                    Communicator comm = new Communicator(ref args, clientProperties);

                    var fact = IServerFactoryPrx.Parse(factoryRef, comm);
                    serverProperties = CreateProperties(defaultProperties, "s_rsa_ca1", "cacert1");
                    serverProperties["IceSSL.TrustOnly"] = "!CN=Client";
                    IServerPrx? server = fact.createServer(serverProperties);
                    try
                    {
                        server!.IcePing();
                        TestHelper.Assert(false);
                    }
                    catch (Exception)
                    {
                    }
                    fact.destroyServer(server);
                    comm.Destroy();
                }
                {
                    clientProperties = CreateProperties(defaultProperties, "c_rsa_ca1", "cacert1");
                    clientProperties["IceSSL.TrustOnly"] = "CN=Client";
                    Communicator comm = new Communicator(ref args, clientProperties);

                    var fact = IServerFactoryPrx.Parse(factoryRef, comm);
                    serverProperties = CreateProperties(defaultProperties, "s_rsa_ca1", "cacert1");
                    IServerPrx? server = fact.createServer(serverProperties);
                    try
                    {
                        server!.IcePing();
                        TestHelper.Assert(false);
                    }
                    catch (Exception)
                    {
                    }
                    fact.destroyServer(server);
                    comm.Destroy();
                }
                {
                    clientProperties = CreateProperties(defaultProperties, "c_rsa_ca1", "cacert1");
                    Communicator comm = new Communicator(ref args, clientProperties);

                    IServerFactoryPrx fact = IServerFactoryPrx.Parse(factoryRef, comm);
                    serverProperties = CreateProperties(defaultProperties, "s_rsa_ca1", "cacert1");
                    serverProperties["IceSSL.TrustOnly"] = "CN=Server";
                    IServerPrx? server = fact.createServer(serverProperties);
                    try
                    {
                        server!.IcePing();
                        TestHelper.Assert(false);
                    }
                    catch (Exception)
                    {
                    }
                    fact.destroyServer(server);
                    comm.Destroy();
                }
                {
                    clientProperties = CreateProperties(defaultProperties, "c_rsa_ca1", "cacert1");
                    clientProperties["IceSSL.TrustOnly"] = "C=Canada,CN=Server";
                    Communicator comm = new Communicator(ref args, clientProperties);

                    var fact = IServerFactoryPrx.Parse(factoryRef, comm);
                    serverProperties = CreateProperties(defaultProperties, "s_rsa_ca1", "cacert1");
                    IServerPrx? server = fact.createServer(serverProperties);
                    try
                    {
                        server!.IcePing();
                        TestHelper.Assert(false);
                    }
                    catch (Exception)
                    {
                    }
                    fact.destroyServer(server);
                    comm.Destroy();
                }
                {
                    clientProperties = CreateProperties(defaultProperties, "c_rsa_ca1", "cacert1");
                    clientProperties["IceSSL.TrustOnly"] = "!C=Canada,CN=Server";
                    Communicator comm = new Communicator(ref args, clientProperties);

                    var fact = IServerFactoryPrx.Parse(factoryRef, comm);
                    serverProperties = CreateProperties(defaultProperties, "s_rsa_ca1", "cacert1");
                    IServerPrx? server = fact.createServer(serverProperties);
                    try
                    {
                        server!.IcePing();
                    }
                    catch (Exception ex)
                    {
                        Console.WriteLine(ex.ToString());
                        TestHelper.Assert(false);
                    }
                    fact.destroyServer(server);
                    comm.Destroy();
                }
                {
                    clientProperties = CreateProperties(defaultProperties, "c_rsa_ca1", "cacert1");
                    clientProperties["IceSSL.TrustOnly"] = "C=Canada;CN=Server";
                    Communicator comm = new Communicator(ref args, clientProperties);

                    var fact = IServerFactoryPrx.Parse(factoryRef, comm);
                    serverProperties = CreateProperties(defaultProperties, "s_rsa_ca1", "cacert1");
                    IServerPrx? server = fact.createServer(serverProperties);
                    try
                    {
                        server!.IcePing();
                    }
                    catch (Exception ex)
                    {
                        Console.WriteLine(ex.ToString());
                        TestHelper.Assert(false);
                    }
                    fact.destroyServer(server);
                    comm.Destroy();
                }
                {
                    clientProperties = CreateProperties(defaultProperties, "c_rsa_ca1", "cacert1");
                    clientProperties["IceSSL.TrustOnly"] = "!C=Canada;!CN=Server";
                    Communicator comm = new Communicator(ref args, clientProperties);

                    var fact = IServerFactoryPrx.Parse(factoryRef, comm);
                    serverProperties = CreateProperties(defaultProperties, "s_rsa_ca1", "cacert1");
                    IServerPrx? server = fact.createServer(serverProperties);
                    try
                    {
                        server!.IcePing();
                        TestHelper.Assert(false);
                    }
                    catch (Exception)
                    {
                    }
                    fact.destroyServer(server);
                    comm.Destroy();
                }
                {
                    clientProperties = CreateProperties(defaultProperties, "c_rsa_ca1", "cacert1");
                    clientProperties["IceSSL.TrustOnly"] = "!CN=Server1"; // Should not match "Server"
                    Communicator comm = new Communicator(ref args, clientProperties);

                    var fact = IServerFactoryPrx.Parse(factoryRef, comm);
                    serverProperties = CreateProperties(defaultProperties, "s_rsa_ca1", "cacert1");
                    IServerPrx? server = fact.createServer(serverProperties);
                    try
                    {
                        server!.IcePing();
                    }
                    catch (Exception ex)
                    {
                        Console.WriteLine(ex.ToString());
                        TestHelper.Assert(false);
                    }
                    fact.destroyServer(server);
                    comm.Destroy();
                }
                {
                    clientProperties = CreateProperties(defaultProperties, "c_rsa_ca1", "cacert1");
                    var comm = new Communicator(ref args, clientProperties);

                    var fact = IServerFactoryPrx.Parse(factoryRef, comm);
                    serverProperties = CreateProperties(defaultProperties, "s_rsa_ca1", "cacert1");
                    serverProperties["IceSSL.TrustOnly"] = "!CN=Client1"; // Should not match "Client"
                    IServerPrx? server = fact.createServer(serverProperties);
                    try
                    {
                        server!.IcePing();
                    }
                    catch (Exception ex)
                    {
                        Console.WriteLine(ex.ToString());
                        TestHelper.Assert(false);
                    }
                    fact.destroyServer(server);
                    comm.Destroy();
                }
                {
                    //
                    // Rejection takes precedence (client).
                    //
                    clientProperties = CreateProperties(defaultProperties, "c_rsa_ca1", "cacert1");
                    clientProperties["IceSSL.TrustOnly"] = "ST=Florida;!CN=Server;C=US";
                    Communicator comm = new Communicator(ref args, clientProperties);

                    var fact = IServerFactoryPrx.Parse(factoryRef, comm);
                    serverProperties = CreateProperties(defaultProperties, "s_rsa_ca1", "cacert1");
                    IServerPrx? server = fact.createServer(serverProperties);
                    try
                    {
                        server!.IcePing();
                        TestHelper.Assert(false);
                    }
                    catch (Exception)
                    {
                    }
                    fact.destroyServer(server);
                    comm.Destroy();
                }
                {
                    //
                    // Rejection takes precedence (server).
                    //
                    clientProperties = CreateProperties(defaultProperties, "c_rsa_ca1", "cacert1");
                    Communicator comm = new Communicator(ref args, clientProperties);

                    var fact = IServerFactoryPrx.Parse(factoryRef, comm);
                    serverProperties = CreateProperties(defaultProperties, "s_rsa_ca1", "cacert1");
                    serverProperties["IceSSL.TrustOnly"] = "C=US;!CN=Client;ST=Florida";
                    IServerPrx? server = fact.createServer(serverProperties);
                    try
                    {
                        server!.IcePing();
                        TestHelper.Assert(false);
                    }
                    catch (Exception)
                    {
                    }
                    fact.destroyServer(server);
                    comm.Destroy();
                }
                Console.Out.WriteLine("ok");

                Console.Out.Write("testing IceSSL.TrustOnly.Client... ");
                Console.Out.Flush();
                {
                    clientProperties = CreateProperties(defaultProperties, "c_rsa_ca1", "cacert1");
                    clientProperties["IceSSL.TrustOnly.Client"] =
                        "C=US, ST=Florida, O=ZeroC\\, Inc.,OU=Ice, emailAddress=info@zeroc.com, CN=Server";
                    var comm = new Communicator(ref args, clientProperties);

                    var fact = IServerFactoryPrx.Parse(factoryRef, comm);
                    serverProperties = CreateProperties(defaultProperties, "s_rsa_ca1", "cacert1");
                    // Should have no effect.
                    serverProperties["IceSSL.TrustOnly.Client"] =
                        "C=US, ST=Florida, O=ZeroC\\, Inc.,OU=Ice, emailAddress=info@zeroc.com, CN=Server";
                    IServerPrx? server = fact.createServer(serverProperties);
                    try
                    {
                        server!.IcePing();
                    }
                    catch (Exception ex)
                    {
                        Console.WriteLine(ex.ToString());
                        TestHelper.Assert(false);
                    }
                    fact.destroyServer(server);
                    comm.Destroy();
                }
                {
                    clientProperties = CreateProperties(defaultProperties, "c_rsa_ca1", "cacert1");
                    clientProperties["IceSSL.TrustOnly.Client"] =
                        "!C=US, ST=Florida, O=ZeroC\\, Inc.,OU=Ice, emailAddress=info@zeroc.com, CN=Server";
                    Communicator comm = new Communicator(ref args, clientProperties);

                    var fact = IServerFactoryPrx.Parse(factoryRef, comm);
                    serverProperties = CreateProperties(defaultProperties, "s_rsa_ca1", "cacert1");
                    IServerPrx? server = fact.createServer(serverProperties);
                    try
                    {
                        server!.IcePing();
                        TestHelper.Assert(false);
                    }
                    catch (Exception)
                    {
                    }
                    fact.destroyServer(server);
                    comm.Destroy();
                }
                {
                    clientProperties = CreateProperties(defaultProperties, "c_rsa_ca1", "cacert1");
                    var comm = new Communicator(ref args, clientProperties);

                    var fact = IServerFactoryPrx.Parse(factoryRef, comm);
                    serverProperties = CreateProperties(defaultProperties, "s_rsa_ca1", "cacert1");
                    // Should have no effect.
                    serverProperties["IceSSL.TrustOnly.Client"] = "!CN=Client";
                    IServerPrx? server = fact.createServer(serverProperties);
                    try
                    {
                        server!.IcePing();
                    }
                    catch (Exception ex)
                    {
                        Console.WriteLine(ex.ToString());
                        TestHelper.Assert(false);
                    }
                    fact.destroyServer(server);
                    comm.Destroy();
                }
                {
                    clientProperties = CreateProperties(defaultProperties, "c_rsa_ca1", "cacert1");
                    clientProperties["IceSSL.TrustOnly.Client"] = "CN=Client";
                    Communicator comm = new Communicator(ref args, clientProperties);

                    var fact = IServerFactoryPrx.Parse(factoryRef, comm);
                    serverProperties = CreateProperties(defaultProperties, "s_rsa_ca1", "cacert1");
                    IServerPrx? server = fact.createServer(serverProperties);
                    try
                    {
                        server!.IcePing();
                        TestHelper.Assert(false);
                    }
                    catch (Exception)
                    {
                    }
                    fact.destroyServer(server);
                    comm.Destroy();
                }
                {
                    clientProperties = CreateProperties(defaultProperties, "c_rsa_ca1", "cacert1");
                    clientProperties["IceSSL.TrustOnly.Client"] = "!CN=Client";
                    Communicator comm = new Communicator(ref args, clientProperties);

                    var fact = IServerFactoryPrx.Parse(factoryRef, comm);
                    serverProperties = CreateProperties(defaultProperties, "s_rsa_ca1", "cacert1");
                    IServerPrx? server = fact.createServer(serverProperties);
                    try
                    {
                        server!.IcePing();
                    }
                    catch (Exception ex)
                    {
                        Console.WriteLine(ex.ToString());
                        TestHelper.Assert(false);
                    }
                    fact.destroyServer(server);
                    comm.Destroy();
                }
                Console.Out.WriteLine("ok");

                Console.Out.Write("testing IceSSL.TrustOnly.Server... ");
                Console.Out.Flush();
                {
                    clientProperties = CreateProperties(defaultProperties, "c_rsa_ca1", "cacert1");
                    // Should have no effect.
                    clientProperties["IceSSL.TrustOnly.Server"] =
                        "C=US, ST=Florida, O=ZeroC\\, Inc.,OU=Ice, emailAddress=info@zeroc.com, CN=Client";
                    Communicator comm = new Communicator(ref args, clientProperties);

                    var fact = IServerFactoryPrx.Parse(factoryRef, comm);
                    serverProperties = CreateProperties(defaultProperties, "s_rsa_ca1", "cacert1");
                    serverProperties["IceSSL.TrustOnly.Server"] =
                        "C=US, ST=Florida, O=ZeroC\\, Inc.,OU=Ice, emailAddress=info@zeroc.com, CN=Client";
                    IServerPrx? server = fact.createServer(serverProperties);
                    try
                    {
                        server!.IcePing();
                    }
                    catch (Exception ex)
                    {
                        Console.WriteLine(ex.ToString());
                        TestHelper.Assert(false);
                    }
                    fact.destroyServer(server);
                    comm.Destroy();
                }
                {
                    clientProperties = CreateProperties(defaultProperties, "c_rsa_ca1", "cacert1");
                    Communicator comm = new Communicator(ref args, clientProperties);

                    var fact = IServerFactoryPrx.Parse(factoryRef, comm);
                    serverProperties = CreateProperties(defaultProperties, "s_rsa_ca1", "cacert1");
                    serverProperties["IceSSL.TrustOnly.Server"] =
                        "!C=US, ST=Florida, O=ZeroC\\, Inc.,OU=Ice, emailAddress=info@zeroc.com, CN=Client";
                    IServerPrx? server = fact.createServer(serverProperties);
                    try
                    {
                        server!.IcePing();
                        TestHelper.Assert(false);
                    }
                    catch (Exception)
                    {
                    }
                    fact.destroyServer(server);
                    comm.Destroy();
                }
                {
                    clientProperties = CreateProperties(defaultProperties, "c_rsa_ca1", "cacert1");
                    // Should have no effect.
                    clientProperties["IceSSL.TrustOnly.Server"] = "!CN=Server";
                    Communicator comm = new Communicator(ref args, clientProperties);

                    var fact = IServerFactoryPrx.Parse(factoryRef, comm);
                    serverProperties = CreateProperties(defaultProperties, "s_rsa_ca1", "cacert1");
                    IServerPrx? server = fact.createServer(serverProperties);
                    try
                    {
                        server!.IcePing();
                    }
                    catch (Exception ex)
                    {
                        Console.WriteLine(ex.ToString());
                        TestHelper.Assert(false);
                    }
                    fact.destroyServer(server);
                    comm.Destroy();
                }
                {
                    clientProperties = CreateProperties(defaultProperties, "c_rsa_ca1", "cacert1");
                    Communicator comm = new Communicator(ref args, clientProperties);

                    IServerFactoryPrx fact = IServerFactoryPrx.Parse(factoryRef, comm);
                    serverProperties = CreateProperties(defaultProperties, "s_rsa_ca1", "cacert1");
                    serverProperties["IceSSL.TrustOnly.Server"] = "CN=Server";
                    IServerPrx? server = fact.createServer(serverProperties);
                    try
                    {
                        server!.IcePing();
                        TestHelper.Assert(false);
                    }
                    catch (Exception)
                    {
                    }
                    fact.destroyServer(server);
                    comm.Destroy();
                }
                {
                    clientProperties = CreateProperties(defaultProperties, "c_rsa_ca1", "cacert1");
                    Communicator comm = new Communicator(ref args, clientProperties);

                    var fact = IServerFactoryPrx.Parse(factoryRef, comm);
                    serverProperties = CreateProperties(defaultProperties, "s_rsa_ca1", "cacert1");
                    serverProperties["IceSSL.TrustOnly.Server"] = "!CN=Client";
                    IServerPrx? server = fact.createServer(serverProperties);
                    try
                    {
                        server!.IcePing();
                        TestHelper.Assert(false);
                    }
                    catch (Exception)
                    {
                    }
                    fact.destroyServer(server);
                    comm.Destroy();
                }
                Console.Out.WriteLine("ok");

                Console.Out.Write("testing IceSSL.TrustOnly.Server.<AdapterName>... ");
                Console.Out.Flush();
                {
                    clientProperties = CreateProperties(defaultProperties, "c_rsa_ca1", "cacert1");
                    Communicator comm = new Communicator(ref args, clientProperties);

                    var fact = IServerFactoryPrx.Parse(factoryRef, comm);
                    serverProperties = CreateProperties(defaultProperties, "s_rsa_ca1", "cacert1");
                    serverProperties["IceSSL.TrustOnly.Server"] = "CN=bogus";
                    serverProperties["IceSSL.TrustOnly.Server.ServerAdapter"] =
                        "C=US, ST=Florida, O=ZeroC\\, Inc.,OU=Ice, emailAddress=info@zeroc.com, CN=Client";
                    IServerPrx? server = fact.createServer(serverProperties);
                    try
                    {
                        server!.IcePing();
                    }
                    catch (Exception ex)
                    {
                        Console.WriteLine(ex.ToString());
                        TestHelper.Assert(false);
                    }
                    fact.destroyServer(server);
                    comm.Destroy();
                }
                {
                    clientProperties = CreateProperties(defaultProperties, "c_rsa_ca1", "cacert1");
                    Communicator comm = new Communicator(ref args, clientProperties);

                    var fact = IServerFactoryPrx.Parse(factoryRef, comm);
                    serverProperties = CreateProperties(defaultProperties, "s_rsa_ca1", "cacert1");
                    serverProperties["IceSSL.TrustOnly.Server.ServerAdapter"] =
                        "!C=US, ST=Florida, O=ZeroC\\, Inc.,OU=Ice, emailAddress=info@zeroc.com, CN=Client";
                    IServerPrx? server = fact.createServer(serverProperties);
                    try
                    {
                        server!.IcePing();
                        TestHelper.Assert(false);
                    }
                    catch (Exception)
                    {
                    }
                    fact.destroyServer(server);
                    comm.Destroy();
                }
                {
                    clientProperties = CreateProperties(defaultProperties, "c_rsa_ca1", "cacert1");
                    Communicator comm = new Communicator(ref args, clientProperties);

                    var fact = IServerFactoryPrx.Parse(factoryRef, comm);
                    serverProperties = CreateProperties(defaultProperties, "s_rsa_ca1", "cacert1");
                    serverProperties["IceSSL.TrustOnly.Server.ServerAdapter"] = "CN=bogus";
                    IServerPrx? server = fact.createServer(serverProperties);
                    try
                    {
                        server!.IcePing();
                        TestHelper.Assert(false);
                    }
                    catch (Exception)
                    {
                    }
                    fact.destroyServer(server);
                    comm.Destroy();
                }
                {
                    clientProperties = CreateProperties(defaultProperties, "c_rsa_ca1", "cacert1");
                    Communicator comm = new Communicator(ref args, clientProperties);

                    var fact = IServerFactoryPrx.Parse(factoryRef, comm);
                    serverProperties = CreateProperties(defaultProperties, "s_rsa_ca1", "cacert1");
                    serverProperties["IceSSL.TrustOnly.Server.ServerAdapter"] = "!CN=bogus";
                    IServerPrx? server = fact.createServer(serverProperties);
                    try
                    {
                        server!.IcePing();
                    }
                    catch (Exception ex)
                    {
                        Console.WriteLine(ex.ToString());
                        TestHelper.Assert(false);
                    }
                    fact.destroyServer(server);
                    comm.Destroy();
                }
                Console.Out.WriteLine("ok");

                Console.Out.Write("testing system CAs... ");
                Console.Out.Flush();
                {
                    //
                    // Retry a few times in case there are connectivity problems with demo.zeroc.com.
                    //
                    const int retryMax = 5;
                    const int retryDelay = 1000;
                    int retryCount = 0;
                    clientProperties = CreateProperties(defaultProperties);
                    clientProperties["IceSSL.DefaultDir"] = "";
                    clientProperties["Ice.Override.Timeout"] = "5000"; // 5s timeout
                    var comm = new Communicator(clientProperties);
                    var p = IObjectPrx.Parse("dummy -p ice1:wss -p 443 -h zeroc.com -r /demo-proxy/chat/glacier2", comm);
                    Console.WriteLine($"protocol {p.Protocol}");
                    while (true)
                    {
                        try
                        {
                            _ = p.GetConnection();
                            break;
                        }
                        catch (Exception ex)
                        {
                            if ((ex is ConnectTimeoutException) ||
                                (ex is TransportException) ||
                                (ex is DNSException))
                            {
                                if (++retryCount < retryMax)
                                {
                                    Console.Out.Write("retrying... ");
                                    Console.Out.Flush();
                                    Thread.Sleep(retryDelay);
                                    continue;
                                }
                            }

                            Console.Out.WriteLine("warning: unable to connect to demo.zeroc.com to check system CA");
                            Console.WriteLine(ex.ToString());
                            break;
                        }
                    }
                    comm.Destroy();
                }
                Console.Out.WriteLine("ok");
            }
            finally
            {
                if (AssemblyUtil.IsWindows && isAdministrator)
                {
                    store.Remove(caCert1);
                    store.Remove(caCert2);
                }
                store.Close();
            }

            return factory;
        }
    }
}
