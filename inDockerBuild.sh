#!/bin/bash

export VERSION=$(cat VERSION)
if [ "${VERSION}x" != "x" ]
then

  (cd ../ITCLib && git pull) && (cd ../ITCFramework && git pull) && \
  (cd ../utils && git pull) && git pull && make CONF=Release.AVX2 clean build install-examples clone-luajit clone-libressl && \
  make build-deb && mv /opt/distrib/lapps-${VERSION}-amd64.deb /opt/lapps/packages/


fi
