import asn1crypto
import http.server
import urllib.parse
from oscrypto import asymmetric
from ocspbuilder import OCSPRequestBuilder, OCSPResponseBuilder
import sys
import base64
import os

db = {}

for name, ca_dir  in [("cacert4", "db/ca4"), ("cai4", "db/ca4/intermediate1")]:
    print("loading {}.der".format(name))
    issuer_cert = asymmetric.load_certificate("{}.der".format(name))
    issuer_key =  asymmetric.load_private_key("{}/ca_key.pem".format(ca_dir), "password")
    print(issuer_cert.asn1.public_key.sha1)

    issuerSha1 = issuer_cert.asn1.public_key.sha1
    db[issuerSha1] = {}
    db[issuerSha1]['issuer_cert'] = issuer_cert
    db[issuerSha1]['issuer_key'] = issuer_key

    certificates = {}
    for filename in os.listdir(ca_dir):
        if filename.endswith(".pem") and not (filename.endswith("_key.pem") or filename == "ca.pem"):
            cert = asymmetric.load_certificate(os.path.join(ca_dir, filename))
            certificates[cert.asn1.serial_number] = cert
    db[issuerSha1]['certificates'] = certificates

    with open("{}/index.txt".format(ca_dir)) as index:
        revocations = {}
        lines = index.readlines()
        for line in lines:
            tokens = line.split('\t')
            if len(tokens) != 6:
                print("invalid line\n" + line)
                syx.exit(1)
            certinfo = {
                "status": tokens[0],
                "revocation_date": asn1crypto.core.UTCTime(tokens[2]),
                "serial_number": int(tokens[3], 16),
            }
            revocations[certinfo["serial_number"]] = certinfo
        db[issuerSha1]['revocations'] = revocations


# /MEowSDBGMEQwQjAJBgUrDgMCGgUABBT+KwqBV0LdnI9wS0u5M5sm6OqKBgQUQr6BG+2RyiUa7G0rdC6B+M0VFxgCCQC9opEJlUmpCQ==

class OCSPHandler(http.server.BaseHTTPRequestHandler):

    def do_GET(self):
        data = base64.b64decode(urllib.parse.unquote(self.path[1:]))
        ocsp_response = None
        try:
            ocsp_request = asn1crypto.ocsp.OCSPRequest.load(data)
            tbs_request = ocsp_request["tbs_request"]
            request_list = tbs_request["request_list"]
            if len(request_list) != 1:
                raise NotImplemented("Combined requests not supported")
            single_request = request_list[0]
            req_cert = single_request["req_cert"]
            serial = req_cert["serial_number"]
            issuer_key_hash = req_cert["issuer_key_hash"]

            issuer = db.get(issuer_key_hash.native)
            if issuer:
                issuer_cert = issuer.get('issuer_cert')
                issuer_key = issuer.get('issuer_key')
                subject_cert = issuer.get('certificates').get(serial.native)
                if subject_cert == None:
                    builder = OCSPResponseBuilder('unauthorized')
                else:
                    cert_info = issuer.get('revocations').get(serial.native)
                    if cert_info == None or cert_info['status'] == 'V':
                        builder = OCSPResponseBuilder('successful', subject_cert, 'good')
                    elif cert_info['status'] == 'R':
                        print("REVOKED ------------->")
                        builder = OCSPResponseBuilder('successful',
                                                      subject_cert,
                                                      'revoked',
                                                      cert_info['revocation_date'].native)
                    else:
                        builder = OCSPResponseBuilder('successful', subject_cert, 'unknown')
                ocsp_response = builder.build(issuer_key, issuer_cert)
            else:
                ocsp_response = OCSPResponseBuilder('unauthorized').build()
        except Exception as e:
            print(e)
            ocsp_response = OCSPResponseBuilder('internal_error').build()

        self.send_response(200)
        self.send_header("Content-Type", "application/ocsp-response")
        self.end_headers()
        self.wfile.write(ocsp_response.dump())

server = http.server.HTTPServer(('127.0.0.1', 20002), OCSPHandler)
server.serve_forever()
