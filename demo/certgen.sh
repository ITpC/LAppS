#!/bin/bash

openssl req -x509 -nodes -new -sha256 -days 1024 -newkey rsa:2048 -keyout localhost.ca.key -out localhost.ca.pem -subj "/C=US/CN=localhost-Root-CA"
openssl x509 -outform pem -in localhost.ca.pem -out localhost.ca.crt


cat << EOM > /tmp/crt.data
authorityKeyIdentifier=keyid,issuer
basicConstraints=CA:FALSE
keyUsage = digitalSignature, nonRepudiation, keyEncipherment, dataEncipherment
subjectAltName = @alt_names
[alt_names]
DNS.1 = localhost
EOM

openssl req -new -nodes -newkey rsa:2048 -keyout localhost.key -out localhost.csr -subj "/C=KZ/ST=Almaty/L=Almaty/O=Localhost-Certificates/CN=localhost"
openssl x509 -req -sha256 -days 1024 -in localhost.csr -CA localhost.ca.pem -CAkey localhost.ca.key -CAcreateserial -extfile /tmp/crt.data -out localhost.crt


#openssl genrsa -out example.org.key 2048
#openssl req -new -key example.org.key -out example.org.csr
#
#openssl genrsa -out ca.key 2048
#openssl req -new -x509 -key ca.key -out ca.crt
#openssl x509 -req -in example.org.csr -CA ca.crt -CAkey ca.key -CAcreateserial -out example.org.crt
#cat example.org.crt ca.crt > example.org.bundle.crt
