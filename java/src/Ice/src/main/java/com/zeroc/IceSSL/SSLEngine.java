//
// Copyright (c) ZeroC, Inc. All rights reserved.
//

package com.zeroc.IceSSL;

import com.zeroc.Ice.InitializationException;
import java.io.InputStream;
import java.security.cert.*;
import java.util.ArrayList;
import java.util.List;
import javax.net.ssl.SNIHostName;
import javax.net.ssl.SNIServerName;
import javax.net.ssl.SSLParameters;

public class SSLEngine {
  public SSLEngine(com.zeroc.Ice.Communicator communicator) {
    _communicator = communicator;
    _logger = _communicator.getLogger();
    _securityTraceLevel =
        _communicator.getProperties().getPropertyAsIntWithDefault("IceSSL.Trace.Security", 0);
    _securityTraceCategory = "Security";
    _trustManager = new TrustManager(_communicator);
  }

  public void initialize() {
    final String prefix = "IceSSL.";
    com.zeroc.Ice.Properties properties = communicator().getProperties();

    //
    // CheckCertName determines whether we compare the name in a peer's
    // certificate against its hostname.
    //
    _checkCertName = properties.getPropertyAsIntWithDefault(prefix + "CheckCertName", 0) > 0;

    //
    // CheckCertName > 1 enables SNI, the SNI extension applies to client
    // connections,
    // indicating the hostname to the server (must be DNS hostname, not an IP
    // address).
    //
    _serverNameIndication = properties.getPropertyAsIntWithDefault(prefix + "CheckCertName", 0) > 1;

    //
    // VerifyPeer determines whether certificate validation failures abort a
    // connection.
    //
    _verifyPeer = properties.getPropertyAsIntWithDefault("IceSSL.VerifyPeer", 2);

    //
    // If the user doesn't supply an SSLContext, we need to create one based
    // on property settings.
    //
    if (_context == null) {
      try {
        //
        // Check for a default directory. We look in this directory for
        // files mentioned in the configuration.
        //
        _defaultDir = properties.getProperty(prefix + "DefaultDir");

        //
        // The keystore holds private keys and associated certificates.
        //
        String keystorePath = properties.getProperty(prefix + "Keystore");

        //
        // The password for the keys.
        //
        String password = properties.getProperty(prefix + "Password");

        //
        // The password for the keystore.
        //
        String keystorePassword = properties.getProperty(prefix + "KeystorePassword");

        //
        // The default keystore type is usually "JKS", but the legal values are
        // determined
        // by the JVM implementation. Other possibilities include "PKCS12" and "BKS".
        //
        final String defaultType = java.security.KeyStore.getDefaultType();
        final String keystoreType =
            properties.getPropertyWithDefault(prefix + "KeystoreType", defaultType);

        //
        // The alias of the key to use in authentication.
        //
        String alias = properties.getProperty(prefix + "Alias");
        boolean overrideAlias = !alias.isEmpty(); // Always use the configured alias

        //
        // The truststore holds the certificates of trusted CAs.
        //
        String truststorePath = properties.getProperty(prefix + "Truststore");

        //
        // The password for the truststore.
        //
        String truststorePassword = properties.getProperty(prefix + "TruststorePassword");

        //
        // The default truststore type is usually "JKS", but the legal values are
        // determined
        // by the JVM implementation. Other possibilities include "PKCS12" and "BKS".
        //
        final String truststoreType =
            properties.getPropertyWithDefault(
                prefix + "TruststoreType", java.security.KeyStore.getDefaultType());

        //
        // Collect the key managers.
        //
        javax.net.ssl.KeyManager[] keyManagers = null;
        java.security.KeyStore keys = null;
        if (_keystoreStream != null || keystorePath.length() > 0) {
          java.io.InputStream keystoreStream = null;
          try {
            if (_keystoreStream != null) {
              keystoreStream = _keystoreStream;
            } else {
              keystoreStream = openResource(keystorePath);
              if (keystoreStream == null) {
                throw new InitializationException("IceSSL: keystore not found:\n" + keystorePath);
              }
            }

            keys = java.security.KeyStore.getInstance(keystoreType);
            char[] passwordChars = null;
            if (keystorePassword.length() > 0) {
              passwordChars = keystorePassword.toCharArray();
            } else if (keystoreType.equals("BKS") || keystoreType.equals("PKCS12")) {
              // Bouncy Castle or PKCS12 does not permit null passwords.
              passwordChars = new char[0];
            }

            keys.load(keystoreStream, passwordChars);

            if (passwordChars != null) {
              java.util.Arrays.fill(passwordChars, '\0');
            }
            keystorePassword = null;
          } catch (java.io.IOException ex) {
            throw new InitializationException(
                "IceSSL: unable to load keystore:\n" + keystorePath, ex);
          } finally {
            if (keystoreStream != null) {
              try {
                keystoreStream.close();
              } catch (java.io.IOException e) {
                // Ignore.
              }
            }
          }

          String algorithm = javax.net.ssl.KeyManagerFactory.getDefaultAlgorithm();
          javax.net.ssl.KeyManagerFactory kmf =
              javax.net.ssl.KeyManagerFactory.getInstance(algorithm);
          char[] passwordChars = new char[0]; // This password cannot be null.
          if (password.length() > 0) {
            passwordChars = password.toCharArray();
          }
          kmf.init(keys, passwordChars);
          if (passwordChars.length > 0) {
            java.util.Arrays.fill(passwordChars, '\0');
          }
          password = null;
          keyManagers = kmf.getKeyManagers();

          //
          // If no alias is specified, we look for the first key entry in the key store.
          //
          // This is required to force the key manager to always choose a certificate
          // even if there's no certificate signed by any of the CA names sent by the
          // server. Ice servers might indeed not always send the CA names of their
          // trusted roots.
          //
          if (alias.isEmpty()) {
            for (java.util.Enumeration<String> e = keys.aliases(); e.hasMoreElements(); ) {
              String a = e.nextElement();
              if (keys.isKeyEntry(a)) {
                alias = a;
                break;
              }
            }
          }

          if (!alias.isEmpty()) {
            //
            // If the user selected a specific alias, we need to wrap the key managers
            // in order to return the desired alias.
            //
            if (!keys.isKeyEntry(alias)) {
              throw new InitializationException(
                  "IceSSL: keystore does not contain an entry with alias `" + alias + "'");
            }

            for (int i = 0; i < keyManagers.length; ++i) {
              keyManagers[i] =
                  new X509KeyManagerI(
                      (javax.net.ssl.X509ExtendedKeyManager) keyManagers[i], alias, overrideAlias);
            }
          }
        }

        //
        // Load the truststore.
        //
        java.security.KeyStore ts = null;
        if (_truststoreStream != null || truststorePath.length() > 0) {
          //
          // If the trust store and the key store are the same input
          // stream or file, don't create another key store.
          //
          if ((_truststoreStream != null && _truststoreStream == _keystoreStream)
              || (truststorePath.length() > 0 && truststorePath.equals(keystorePath))) {
            assert keys != null;
            ts = keys;
          } else {
            java.io.InputStream truststoreStream = null;
            try {
              if (_truststoreStream != null) {
                truststoreStream = _truststoreStream;
              } else {
                truststoreStream = openResource(truststorePath);
                if (truststoreStream == null) {
                  throw new InitializationException(
                      "IceSSL: truststore not found:\n" + truststorePath);
                }
              }

              ts = java.security.KeyStore.getInstance(truststoreType);

              char[] passwordChars = null;
              if (truststorePassword.length() > 0) {
                passwordChars = truststorePassword.toCharArray();
              } else if (truststoreType.equals("BKS") || truststoreType.equals("PKCS12")) {
                // Bouncy Castle or PKCS12 does not permit null passwords.
                passwordChars = new char[0];
              }

              ts.load(truststoreStream, passwordChars);

              if (passwordChars != null) {
                java.util.Arrays.fill(passwordChars, '\0');
              }
              truststorePassword = null;
            } catch (java.io.IOException ex) {
              throw new InitializationException(
                  "IceSSL: unable to load truststore:\n" + truststorePath, ex);
            } finally {
              if (truststoreStream != null) {
                try {
                  truststoreStream.close();
                } catch (java.io.IOException e) {
                  // Ignore.
                }
              }
            }
          }
        }

        //
        // Collect the trust managers. Use IceSSL.Truststore if
        // specified, otherwise use the Java root CAs if
        // Ice.Use.PlatformCAs is enabled. If none of these are enabled,
        // use the keystore or a dummy trust manager which rejects any
        // certificate.
        //
        javax.net.ssl.TrustManager[] trustManagers = null;
        {
          String algorithm = javax.net.ssl.TrustManagerFactory.getDefaultAlgorithm();
          javax.net.ssl.TrustManagerFactory tmf =
              javax.net.ssl.TrustManagerFactory.getInstance(algorithm);
          java.security.KeyStore trustStore = null;
          if (ts != null) {
            trustStore = ts;
          } else if (properties.getPropertyAsInt("IceSSL.UsePlatformCAs") <= 0) {
            if (keys != null) {
              trustStore = keys;
            } else {
              trustManagers =
                  new javax.net.ssl.TrustManager[] {
                    new javax.net.ssl.X509TrustManager() {
                      @Override
                      public void checkClientTrusted(X509Certificate[] chain, String authType)
                          throws CertificateException {
                        throw new CertificateException("no trust anchors");
                      }

                      @Override
                      public void checkServerTrusted(X509Certificate[] chain, String authType)
                          throws CertificateException {
                        throw new CertificateException("no trust anchors");
                      }

                      @Override
                      public X509Certificate[] getAcceptedIssuers() {
                        return new X509Certificate[0];
                      }
                    }
                  };
            }
          } else {
            trustStore = null;
          }

          //
          // Attempting to establish an outgoing connection with an empty truststore can
          // cause hangs that eventually result in an exception such as:
          //
          // java.security.InvalidAlgorithmParameterException: the trustAnchors parameter
          // must be non-empty
          //
          if (trustStore != null && trustStore.size() == 0) {
            throw new InitializationException("IceSSL: truststore is empty");
          }

          if (trustManagers == null) {
            tmf.init(trustStore);
            trustManagers = tmf.getTrustManagers();
          }
          assert (trustManagers != null);
        }

        //
        // Initialize the SSL context.
        //
        _context = javax.net.ssl.SSLContext.getInstance("TLS");
        _context.init(keyManagers, trustManagers, null);
      } catch (java.security.GeneralSecurityException ex) {
        throw new InitializationException("IceSSL: unable to initialize context", ex);
      }
    }

    //
    // Clear cached input streams.
    //
    _keystoreStream = null;
    _truststoreStream = null;
  }

  int securityTraceLevel() {
    return _securityTraceLevel;
  }

  String securityTraceCategory() {
    return _securityTraceCategory;
  }

  javax.net.ssl.SSLEngine createSSLEngine(boolean incoming, String host, int port) {
    javax.net.ssl.SSLEngine engine;
    try {
      if (host != null) {
        engine = _context.createSSLEngine(host, port);
      } else {
        engine = _context.createSSLEngine();
      }
      engine.setUseClientMode(!incoming);
    } catch (Exception ex) {
      throw new com.zeroc.Ice.SecurityException("IceSSL: couldn't create SSL engine", ex);
    }

    if (incoming) {
      if (_verifyPeer == 0) {
        engine.setWantClientAuth(false);
        engine.setNeedClientAuth(false);
      } else if (_verifyPeer == 1) {
        engine.setWantClientAuth(true);
      } else {
        engine.setNeedClientAuth(true);
      }
    } else {
      //
      // Enable the HTTPS hostname verification algorithm
      //
      if (_checkCertName && _verifyPeer > 0 && host != null) {
        SSLParameters params = new SSLParameters();
        params.setEndpointIdentificationAlgorithm("HTTPS");
        engine.setSSLParameters(params);
      }
    }

    // Server name indication
    if (!incoming && _serverNameIndication) {
      SNIHostName serverName = null;
      try {
        serverName = new SNIHostName(host);
        SSLParameters sslParams = engine.getSSLParameters();
        List<SNIServerName> serverNames = new ArrayList<>();
        serverNames.add(serverName);
        sslParams.setServerNames(serverNames);
        engine.setSSLParameters(sslParams);
      } catch (IllegalArgumentException ex) {
        // Invalid SNI hostname, ignore because it might be an IP
      }
    }

    return engine;
  }

  void traceConnection(String desc, javax.net.ssl.SSLEngine engine, boolean incoming) {
    javax.net.ssl.SSLSession session = engine.getSession();
    String msg =
        "SSL summary for "
            + (incoming ? "incoming" : "outgoing")
            + " connection\n"
            + "cipher = "
            + session.getCipherSuite()
            + "\n"
            + "protocol = "
            + session.getProtocol()
            + "\n"
            + desc;
    _logger.trace(_securityTraceCategory, msg);
  }

  com.zeroc.Ice.Communicator communicator() {
    return _communicator;
  }

  void verifyPeer(String address, ConnectionInfo info, String desc) {
    //
    // IceSSL.VerifyPeer is translated into the proper SSLEngine configuration
    // for a server, but we have to do it ourselves for a client.
    //
    if (!info.incoming) {
      if (_verifyPeer > 0 && !info.verified) {
        throw new com.zeroc.Ice.SecurityException("IceSSL: server did not supply a certificate");
      }
    }

    if (!_trustManager.verify(info, desc)) {
      String msg =
          (info.incoming ? "incoming" : "outgoing")
              + " connection rejected by trust manager\n"
              + desc;
      if (_securityTraceLevel >= 1) {
        _logger.trace(_securityTraceCategory, msg);
      }
      com.zeroc.Ice.SecurityException ex = new com.zeroc.Ice.SecurityException();
      ex.reason = msg;
      throw ex;
    }
  }

  void trustManagerFailure(boolean incoming, CertificateException ex) throws CertificateException {
    if (_verifyPeer == 0) {
      if (_securityTraceLevel >= 1) {
        String msg = "ignoring peer verification failure";
        if (_securityTraceLevel > 1) {
          java.io.StringWriter sw = new java.io.StringWriter();
          java.io.PrintWriter pw = new java.io.PrintWriter(sw);
          ex.printStackTrace(pw);
          pw.flush();
          msg += ":\n" + sw.toString();
        }
        _logger.trace(_securityTraceCategory, msg);
      }
    } else {
      throw ex;
    }
  }

  private java.io.InputStream openResource(String path) throws java.io.IOException {
    boolean isAbsolute = false;
    try {
      new java.net.URL(path);
      isAbsolute = true;
    } catch (java.net.MalformedURLException ex) {
      java.io.File f = new java.io.File(path);
      isAbsolute = f.isAbsolute();
    }

    java.io.InputStream stream =
        com.zeroc.IceInternal.Util.openResource(getClass().getClassLoader(), path);

    //
    // If the first attempt fails and IceSSL.DefaultDir is defined and the original
    // path is
    // relative,
    // we prepend the default directory and try again.
    //
    if (stream == null && _defaultDir.length() > 0 && !isAbsolute) {
      stream =
          com.zeroc.IceInternal.Util.openResource(
              getClass().getClassLoader(), _defaultDir + java.io.File.separator + path);
    }

    if (stream != null) {
      stream = new java.io.BufferedInputStream(stream);
    }

    return stream;
  }

  private com.zeroc.Ice.Communicator _communicator;
  private com.zeroc.Ice.Logger _logger;
  private int _securityTraceLevel;
  private String _securityTraceCategory;
  private javax.net.ssl.SSLContext _context;
  private String _defaultDir;
  private boolean _checkCertName;
  private boolean _serverNameIndication;
  private int _verifyPeer;
  private TrustManager _trustManager;
  private InputStream _keystoreStream;
  private InputStream _truststoreStream;
}
