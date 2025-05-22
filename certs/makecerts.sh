#!/bin/bash
set -eux  # Exit on error, print commands

COMMON_CA=common/ca/ca.cnf

CONFIGURATION_CAS=$(find configuration -name 'ca*.cnf')
INTERMEDIATE_CAS=$(find configuration -name 'i*.cnf')

CLIENT_CERTS=$(find . -name 'client*.cnf')
SERVER_CERTS=$(find . -name 'server*.cnf')

ROOT_CAS="${COMMON_CA} ${CONFIGURATION_CAS}"

# Default days for certificates, 398 for compatibility with macOs requirements
default_days=398

# Default password for PKCS12 files
default_password="password"

# -legacy is needed for compatibility macOS Keychain, it doesn't support OpenSSL 3.0 defaults
common_pkcs12_args="-passout pass:${default_password} -legacy -keypbe PBE-SHA1-3DES -certpbe PBE-SHA1-3DES -macalg sha1"

init_ca_dir(){
    outputdir=$1
    # Remove existing files
    rm -rf ${outputdir}/newcerts ${outputdir}/crl ${outputdir}/index.txt* ${outputdir}/serial* ${outputdir}/crlnumber

    # Create the required directories
    mkdir -p ${outputdir}/newcerts ${outputdir}/crl
    touch ${outputdir}/index.txt
    echo "1000" > ${outputdir}/serial
    echo "1000" > ${outputdir}/crlnumber
}

# If the filename contains "expired" or "notyet", run the command with date time adjusted
# by faketime, otherwise run the command normally.
run_with_faketime_if_needed() {
    local filename="$1"
    shift
    local cmd=("$@")

    local offset=""
    if [[ "$filename" == *expired* ]]; then
        offset="-$((default_days + 1))d"
    elif [[ "$filename" == *notyet* ]]; then
        offset="+$((default_days + 1))d"
    fi

    if [[ -n "$offset" ]]; then
        faketime -f "$offset" "${cmd[@]}"
    else
        "${cmd[@]}"
    fi
}

# Create self-signed root CA certificates
for ca in ${ROOT_CAS}; do
    outputdir=$(dirname ${ca})
    alias=$(basename ${outputdir})
    cert_file=${outputdir}/$(basename ${ca} .cnf)_cert.pem
    cert_der_file=${outputdir}/$(basename ${ca} .cnf)_cert.der
    key_file=${outputdir}/$(basename ${ca} .cnf)_key.pem
    pkcs12_file=${outputdir}/$(basename ${ca} .cnf).p12

    init_ca_dir ${outputdir}

    openssl req -x509 -nodes -days ${default_days} \
        -keyout ${key_file} \
        -out ${cert_file} \
        -extensions v3_extensions \
        -config ${ca}

    # Export as DER
    openssl x509 -in ${cert_file} -out ${cert_der_file} -outform DER

    # Create a PKCS12 file for the root CA
    openssl pkcs12 -export -out ${pkcs12_file} -inkey ${key_file} -in ${cert_file} \
        -name ${alias} ${common_pkcs12_args}
done

# Create Intermediate CA certificates signed by its parent CA
for i in ${INTERMEDIATE_CAS}; do
    outputdir=$(dirname ${i})
    alias=$(basename ${i} .cnf)
    ca_dir=$(dirname ${outputdir})
    ca_config=${ca_dir}/$(basename ${ca_dir}).cnf
    ca_cert=${ca_dir}/$(basename ${ca_dir})_cert.pem
    csr_file=${outputdir}/$(basename ${i} .cnf).csr
    cert_file=${outputdir}/$(basename ${i} .cnf)_cert.pem
    key_file=${outputdir}/$(basename ${i} .cnf)_key.pem
    pkcs12_file=${outputdir}/$(basename ${i} .cnf).p12
    jks_file=${outputdir}/$(basename ${i} .cnf).jks

    init_ca_dir ${outputdir}

    echo "Creating intermediate CA csr ${csr_file} and key ${key_file}"
    openssl req -new -nodes -out ${csr_file} -keyout ${key_file} -config ${i}

    # Sign the intermediate CA with its parent CA
    cmd=(openssl ca -config ${ca_config} -in ${csr_file} -out ${cert_file} \
        -extfile ${i} -extensions v3_extensions -batch)
    run_with_faketime_if_needed "${cert_file}" "${cmd[@]}"
done

# Create client and server certificates signed by its corresponding CA
for i in ${CLIENT_CERTS} ${SERVER_CERTS}; do
    outputdir=$(dirname ${i})
    alias=$(basename ${i} .cnf)
    ca_config=${outputdir}/$(basename ${outputdir}).cnf
    ca_cert=${outputdir}/$(basename ${outputdir})_cert.pem
    ca_crl=${outputdir}/$(basename ${outputdir}).crl.pem
    csr_file=${outputdir}/$(basename ${i} .cnf).csr
    cert_file=${outputdir}/$(basename ${i} .cnf)_cert.pem
    key_file=${outputdir}/$(basename ${i} .cnf)_key.pem
    pkcs12_file=${outputdir}/$(basename ${i} .cnf).p12
    jks_file=${outputdir}/$(basename ${i} .cnf).jks

    echo "Creating csr ${csr_file} and key ${key_file}"
    openssl req -new -nodes -out ${csr_file} -keyout ${key_file} -config ${i}

    cmd=(openssl ca -config ${ca_config} -in ${csr_file} -out ${cert_file} \
        -extfile ${i} -extensions v3_extensions -batch)
    run_with_faketime_if_needed "${cert_file}" "${cmd[@]}"

    # Export as PCKS12
    rm -f ${pkcs12_file}
    openssl pkcs12 -export -out ${pkcs12_file} -inkey ${key_file} -in ${cert_file} -certfile ${ca_cert} \
        -name ${alias} ${common_pkcs12_args}

    # Export PKCS12 as JKS
    rm -f ${jks_file}
    keytool -importkeystore -srckeystore ${pkcs12_file} -srcstoretype PKCS12 -destkeystore ${jks_file} \
        -deststoretype JKS -srcstorepass ${default_password} -deststorepass ${default_password}
done


revoke_certificates(){
    ca_config=$1
    cert_files=$2

    outputdir=$(dirname ${ca_config})
    ca_crl=${outputdir}/$(basename ${outputdir}).crl.pem

    for i in ${cert_files}; do
        openssl ca -config ${ca_config} -revoke ${i} -passin pass:${default_password} -batch
    done
    openssl ca -config ${ca_config} -gencrl -out ${ca_crl} -crldays ${default_days} -passin pass:${default_password}
}

# Revoke client and server certificates using in revocation tests
revoke_certificates configuration//ca3/ca3.cnf \
    "configuration/ca3/server_revoked_cert.pem configuration/ca3/i1/i1_cert.pem"

revoke_certificates configuration/ca3/i1/i1.cnf configuration/ca3/i1/server_revoked_cert.pem

revoke_certificates configuration/ca4/ca4.cnf \
    "configuration/ca4/server_revoked_cert.pem configuration/ca4/i1/i1_cert.pem"

# Create ca_all_cert.pem with all configuration CA certificates
CERT_FILES=(${CONFIGURATION_CAS})
CERT_FILES=("${CERT_FILES[@]/%.cnf/_cert.pem}")
cat ${CERT_FILES[@]} > configuration/ca_all_cert.pem
