import asn1crypto
from bottle import run, get, post, HTTPResponse
from oscrypto import asymmetric
from ocspbuilder import OCSPRequestBuilder, OCSPResponseBuilder
import sys
import base64

certs = {}

for name, ca_dir  in [("cacert4", "db/ca4"), ("cai4", "db/ca4/intermediate1")]:
    issuer_cert = asymmetric.load_certificate("{}.der".format(name))
    print(issuer_cert.asn1.public_key.sha1)

    with open("{}/index.txt".format(ca_dir)) as index:
        lines = index.readlines()
        for line in lines:
            tokens = line.split('\t')
            if len(tokens) != 6:
                print("invalid line\n" + line)
                syx.exit(1)
                certinfo = {
                    "status": tokens[0],
                    "expiration_date": tokens[1],
                    "revocation_date": tokens[2],
                    "serial_number": tokens[3],
                    "dn": tokens[5]
                }
                print(certinfo["serial_number"])
                certs[certinfo["serial_number"]] = certinfo

@post("/status")
def statusPOST():
    return status(request.body.read())

@get("/status/<data>")
def statusGET(data):
    return status(base64.b64decode("ME8wTaADAgEAMEYwRDBCMAkGBSsOAwIaBQAEFBrO9RuYYk+S5mILpvPpbf3/eKuyBBQeB34c4jGmdqsXmuCOGC06nHryGQIJAK9AUcy55T0A"))

def status(data):
    try:
        ocsp_request = asn1crypto.ocsp.OCSPRequest.load(data)
        tbs_request = ocsp_request["tbs_request"]
        request_list = tbs_request["request_list"]
        if len(request_list) != 1:
            raise NotImplemented("Combined requests not supported")
        single_request = request_list[0]
        req_cert = single_request["req_cert"]
        serial = req_cert["serial_number"]
        key_alg = req_cert["hash_algorithm"]["algorithm"]
        issuer_key_hash = req_cert["issuer_key_hash"]
        print(serial)
        print(serial.native)
        print(key_alg.native)
        print(issuer_key_hash.native)
        return "Hello"
    except Exception as e:
        print(e)
        return HTTPResponse(
            status=200,
            body="TODO",
            content_type='application/ocsp-response',
        )

run(host='localhost', port=8080, debug=True)

