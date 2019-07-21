#!/bin/bash

openssl genrsa -out example.org.key 2048
openssl req -new -key example.org.key -out example.org.csr

openssl genrsa -out ca.key 2048
openssl req -new -x509 -key ca.key -out ca.crt
openssl x509 -req -in example.org.csr -CA ca.crt -CAkey ca.key -CAcreateserial -out example.org.crt
cat example.org.crt ca.crt > example.org.bundle.crt
