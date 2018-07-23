#!/bin/bash

export VERSION=$(cat VERSION)
if [ "${VERSION}x" != "x" ]
then

  (cd ../ITCLib && git pull) && (cd ../ITCFramework && git pull) && \
  (cd ../utils && git pull) && (cd ../lar && git pull && make install)  && git pull && make CONF=Release.GENERIC clean build install-examples clone-luajit clone-libressl && \
  make build-deb && mv /opt/distrib/lapps-${VERSION}-amd64.deb /opt/lapps/packages/

fi
