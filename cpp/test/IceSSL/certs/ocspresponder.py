from datetime import datetime, timedelta, timezone
import asn1crypto
import http.server
import urllib.parse
from oscrypto import asymmetric
from ocspbuilder import OCSPResponseBuilder
import sys
import base64
import os
import traceback

db = {}

for name, ca_dir, certs in [("cacert4", "db/ca4", ["s_rsa_ca4.pem",
                                                   "s_rsa_ca4_revoked.pem",
                                                   "intermediate1/ca.pem"]),
                            ("cai4", "db/ca4/intermediate1", ["s_rsa_cai4.pem",
                                                              "s_rsa_cai4_revoked.pem"])]:
    print("loading {}.der".format(name))
    issuer_cert = asymmetric.load_certificate("{}.der".format(name))
    issuer_key = asymmetric.load_private_key("{}/ca_key.pem".format(ca_dir), "password")
    print(issuer_cert.asn1.public_key.sha1)

    issuerSha1 = issuer_cert.asn1.public_key.sha1
    db[issuerSha1] = {}
    db[issuerSha1]['issuer_cert'] = issuer_cert
    db[issuerSha1]['issuer_key'] = issuer_key

    certificates = {}
    for filename in certs:
        cert = asymmetric.load_certificate(os.path.join(ca_dir, filename))
        print("load {} serial {}".format(filename, cert.asn1.serial_number))
        certificates[cert.asn1.serial_number] = cert
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
                "revocation_date": asn1crypto.core.UTCTime(tokens[2]),
                "serial_number": int(tokens[3], 16),
            }
            revocations[certinfo["serial_number"]] = certinfo
        db[issuerSha1]['revocations'] = revocations

class OCSPHandler(http.server.BaseHTTPRequestHandler):

    def do_POST(self):
        length = int(self.headers['Content-Length'])
        data = self.rfile.read(length)
        self.validate(data)

    def do_GET(self):
        data = base64.b64decode(urllib.parse.unquote(self.path[1:]))
        self.validate(data)

    def validate(self, data):
        ocsp_response = None
        try:
            ocsp_request = asn1crypto.ocsp.OCSPRequest.load(data)
            tbs_request = ocsp_request["tbs_request"]
            request_list = tbs_request["request_list"]
            if len(request_list) != 1:
                raise Exception("Combined requests not supported")
            single_request = request_list[0]
            req_cert = single_request["req_cert"]
            serial = req_cert["serial_number"]
            issuer_key_hash = req_cert["issuer_key_hash"]

            issuer = db.get(issuer_key_hash.native)
            if issuer:
                issuer_cert = issuer.get('issuer_cert')
                issuer_key = issuer.get('issuer_key')
                subject_cert = issuer.get('certificates').get(serial.native)
                if subject_cert is None:
                    print("UNAUTHORIZED 1 ------------->")
                    builder = OCSPResponseBuilder('unauthorized')
                else:
                    cert_info = issuer.get('revocations').get(serial.native)
                    if cert_info is None or cert_info['status'] == 'V':
                        print("GOOD ------------->")
                        builder = OCSPResponseBuilder('successful', subject_cert, 'good')
                    elif cert_info['status'] == 'R':
                        print("REVOKED ------------->")
                        revocation_time = datetime(2021, 9, 16, 00, 0, 0, tzinfo=timezone.utc)
                        builder = OCSPResponseBuilder('successful',
                                                      subject_cert,
                                                      'revoked',
                                                      revocation_time)
                    else:
                        print("UNKNOWN ------------->")
                        builder = OCSPResponseBuilder('successful', subject_cert, 'unknown')
                if ocsp_request.nonce_value:
                    builder.nonce = ocsp_request.nonce_value.native
                builder.this_update = datetime(2021, 9, 16, 00, 0, 0, tzinfo=timezone.utc)
                builder.next_update = datetime(2021, 9, 17, 14, 0, 0, tzinfo=timezone.utc)
                ocsp_response = builder.build(issuer_key, issuer_cert)
            else:
                print("UNAUTHORIZED 2 ------------->")
                print("issuer: ")
                print(issuer_key_hash.native)
                print(req_cert["hash_algorithm"].native)
                ocsp_response = OCSPResponseBuilder('unauthorized').build()
        except Exception as e:
            print("ERROR ------------->")
            traceback.print_exc(file=sys.stdout)
            ocsp_response = OCSPResponseBuilder('internal_error').build()

        self.send_response(200)
        self.send_header("Content-Type", "application/ocsp-response")
        self.end_headers()
        self.wfile.write(ocsp_response.dump())


server = http.server.HTTPServer(('127.0.0.1', 20002), OCSPHandler)
server.serve_forever()
