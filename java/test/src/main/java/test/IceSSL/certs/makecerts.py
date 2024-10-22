#!/usr/bin/env python3
#
# Copyright (c) ZeroC, Inc. All rights reserved.
#

import os
import sys
import getopt

try:
    import IceCertUtils
except ImportError:
    print(
        "error: couldn't find IceCertUtils, install `zeroc-icecertutils' package "
        "from Python package repository"
    )
    sys.exit(1)

toplevel = "."
while os.path.abspath(toplevel) != "/":
    toplevel = os.path.normpath(os.path.join("..", toplevel))
    if os.path.exists(os.path.join(toplevel, "scripts", "Util.py")):
        break
else:
    raise RuntimeError("can't find toplevel directory!")

cppcerts = os.path.join(toplevel, "cpp", "test", "IceSSL", "certs")
if not os.path.exists(os.path.join(cppcerts, "db", "ca1", "ca.pem")):
    print(
        "error: CA database is not initialized in `"
        + os.path.join(cppcerts, "db")
        + "',"
        " run makecerts.py in `" + cppcerts + "' first"
    )
    sys.exit(1)


def usage():
    print("Usage: " + sys.argv[0] + " [options]")
    print("")
    print("Options:")
    print("-h               Show this message.")
    print("-d | --debug     Debugging output.")
    print("--force          Re-save all the files even if they already exists.")
    sys.exit(1)


#
# Check arguments
#
debug = False
force = False
try:
    opts, args = getopt.getopt(sys.argv[1:], "hd", ["help", "debug", "force"])
except getopt.GetoptError as e:
    print("Error %s " % e)
    usage()
    sys.exit(1)

for o, a in opts:
    if o == "-h" or o == "--help":
        usage()
        sys.exit(0)
    elif o == "-d" or o == "--debug":
        debug = True
    elif o == "--force":
        force = True

ca1 = IceCertUtils.CertificateFactory(
    home=os.path.join(cppcerts, "db", "ca1"), debug=debug
)
ca2 = IceCertUtils.CertificateFactory(
    home=os.path.join(cppcerts, "db", "ca2"), debug=debug
)
cai1 = ca1.getIntermediateFactory("intermediate1")
cai2 = cai1.getIntermediateFactory("intermediate1")

if force or not os.path.exists("cacert1.jks"):
    ca1.getCA().save("cacert1.jks")
if force or not os.path.exists("cacert2.jks"):
    ca2.getCA().save("cacert2.jks")

certs = [
    (ca1, "s_rsa_ca1", None, {}),
    (ca1, "c_rsa_ca1", None, {}),
    (ca1, "s_rsa_ca1_exp", None, {}),  # Expired certificate
    (ca1, "c_rsa_ca1_exp", None, {}),  # Expired certificate
    (ca1, "s_rsa_ca1_cn1", None, {}),
    (ca1, "s_rsa_ca1_cn2", None, {}),
    (ca1, "s_rsa_ca1_cn3", None, {}),
    (ca1, "s_rsa_ca1_cn4", None, {}),
    (ca1, "s_rsa_ca1_cn5", None, {}),
    (ca1, "s_rsa_ca1_cn6", None, {}),
    (ca1, "s_rsa_ca1_cn7", None, {}),
    (ca1, "s_rsa_ca1_cn8", None, {}),
    (ca2, "s_rsa_ca2", None, {}),
    (ca2, "c_rsa_ca2", None, {}),
    (cai1, "s_rsa_cai1", None, {}),
    (cai2, "s_rsa_cai2", None, {}),
    (cai2, "c_rsa_cai2", None, {}),
    (ca1, "s_rsa_ca1", "s_rsa_wroot_ca1", {"root": True}),
]

#
# Save the certificate JKS files.
#
for ca, alias, path, args in certs:
    if not path:
        path = alias
    cert = ca.get(alias)
    if force or not os.path.exists(path + ".jks"):
        cert.save(path + ".jks", alias="cert", **args)

#
# Create a cacerts.jks truststore that contains both CA certs.
#
if force or not os.path.exists("cacerts.jks"):
    if os.path.exists("cacerts.jks"):
        os.remove("cacerts.jks")
    ca1.getCA().exportToKeyStore("cacerts.jks", alias="cacert1")
    ca2.getCA().exportToKeyStore("cacerts.jks", alias="cacert2")

if force or not os.path.exists("s_cacert2.jks"):
    ca2.getCA().saveJKS("s_cacert2.jks", addkey=True)
