from datetime import datetime, timedelta, timezone
from cryptography.x509 import ocsp, load_pem_x509_certificate, ReasonFlags, SubjectKeyIdentifier
from cryptography.hazmat.primitives import hashes, serialization
from cryptography.hazmat.primitives.serialization import Encoding
from cryptography.hazmat.primitives.serialization import PublicFormat
import http.server
import urllib.parse
import sys
import base64
import os
import traceback
import hashlib
from oscrypto import asymmetric


def load_certificate(path):
    with open(path, 'rb') as f:
        return load_pem_x509_certificate(f.read())


def load_private_key(path, password):
    with open(path, 'rb') as f:
        return serialization.load_pem_private_key(f.read(), password)


def load_db():
    db = {}
    for ca_dir, certs in [("db/ca4", ["s_rsa_ca4.pem",
                                      "s_rsa_ca4_revoked.pem",
                                      "intermediate1/ca.pem"]),
                          ("db/ca4/intermediate1", ["s_rsa_cai4.pem", "s_rsa_cai4_revoked.pem"])]:
        issuer_cert = load_certificate("{}/ca.pem".format(ca_dir))
        issuer_key = load_private_key("{}/ca_key.pem".format(ca_dir), b"password")

        issuerSha1 = issuer_cert.extensions.get_extension_for_class(SubjectKeyIdentifier).value.digest

        db[issuerSha1] = {}
        db[issuerSha1]['issuer_cert'] = issuer_cert
        db[issuerSha1]['issuer_key'] = issuer_key

        certificates = {}
        for filename in certs:
            cert = load_certificate(os.path.join(ca_dir, filename))
            certificates[cert.serial_number] = cert
        db[issuerSha1]['certificates'] = certificates

        with open("{}/index.txt".format(ca_dir)) as index:
            revocations = {}
            lines = index.readlines()
            for line in lines:
                tokens = line.split('\t')
                if len(tokens) != 6:
                    print("invalid line\n" + line)
                    sys.exit(1)
                certinfo = {
                    "status": tokens[0],
                    "revocation_time": datetime.strptime(tokens[2], "%y%m%d%H%M%S%z"),
                    "serial_number": int(tokens[3], 16),
                }
                revocations[certinfo["serial_number"]] = certinfo
            db[issuerSha1]['revocations'] = revocations
    return db


db = load_db()


class OCSPHandler(http.server.BaseHTTPRequestHandler):

    def do_POST(self):
        length = int(self.headers['Content-Length'])
        data = self.rfile.read(length)
        self.validate(data)

    def do_GET(self):
        data = base64.b64decode(urllib.parse.unquote(self.path[1:]))
        self.validate(data)

    def validate(self, data):
        response = None
        this_update = datetime.now(timezone.utc)
        next_update = this_update + timedelta(seconds=60)
        try:
            request = ocsp.load_der_ocsp_request(data)
            serial = request.serial_number
            issuer_key_hash = request.issuer_key_hash

            issuer = db.get(issuer_key_hash)
            if issuer:
                issuer_cert = issuer.get('issuer_cert')
                issuer_key = issuer.get('issuer_key')
                subject_cert = issuer.get('certificates').get(serial)
                if subject_cert is None:
                    print("UNAUTHORIZED 1 ------------->")
                    response = ocsp.OCSPResponseBuilder.build_unsuccessful(ocsp.OCSPResponseStatus.UNAUTHORIZED)
                else:
                    cert_info = issuer.get('revocations').get(serial)
                    builder = ocsp.OCSPResponseBuilder()
                    if cert_info is None or cert_info['status'] == 'V':
                        print("GOOD ------------->")
                        builder = builder.add_response(cert=subject_cert,
                                                       issuer=issuer_cert,
                                                       algorithm=hashes.SHA1(),
                                                       cert_status=ocsp.OCSPCertStatus.GOOD,
                                                       this_update=this_update,
                                                       next_update=next_update,
                                                       revocation_time=None,
                                                       revocation_reason=None)
                    elif cert_info['status'] == 'R':
                        print("REVOKED ------------->")
                        builder = builder.add_response(cert=subject_cert,
                                                       issuer=issuer_cert,
                                                       algorithm=hashes.SHA1(),
                                                       cert_status=ocsp.OCSPCertStatus.REVOKED,
                                                       this_update=this_update,
                                                       next_update=next_update,
                                                       revocation_time=cert_info['revocation_time'],
                                                       revocation_reason=ReasonFlags.unspecified)
                    else:
                        print("UNKNOWN ------------->")
                        builder = builder.add_response(cert=subject_cert,
                                                       issuer=issuer_cert,
                                                       algorithm=hashes.SHA1(),
                                                       cert_status=ocsp.OCSPCertStatus.UNKNOWN,
                                                       this_update=this_update,
                                                       next_update=next_update,
                                                       revocation_time=None,
                                                       revocation_reason=None)
                    builder = builder.responder_id(ocsp.OCSPResponderEncoding.HASH, issuer_cert)
                response = builder.sign(issuer_key, hashes.SHA256())
            else:
                print("UNAUTHORIZED 2 ------------->")
                print(issuer_key_hash)
                response = ocsp.OCSPResponseBuilder.build_unsuccessful(ocsp.OCSPResponseStatus.UNAUTHORIZED)
        except Exception as e:
            print("ERROR ------------->")
            traceback.print_exc(file=sys.stdout)
            response = ocsp.OCSPResponseBuilder.build_unsuccessful(ocsp.OCSPResponseStatus.UNAUTHORIZED)

        self.send_response(200)
        self.send_header("Content-Type", "application/ocsp-response")
        self.end_headers()
        self.wfile.write(response.public_bytes(Encoding.DER))


server = http.server.HTTPServer(('127.0.0.1', 20002), OCSPHandler)
server.serve_forever()
