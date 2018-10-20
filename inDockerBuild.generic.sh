#!/bin/bash

export VERSION=$(cat VERSION)
if [ "${VERSION}x" != "x" ]
then

  (cd ../ITCLib && git pull) && (cd ../ITCFramework && git pull) && \
  (cd ../utils && git pull) && (cd ../lar && git pull && make install)  && git pull &&\
  rm -f /opt/lapps/bin/lapps* &&\
  make CONF=Release.GENERIC clean build &&\
  make CONF=Release.GENERIC.NO_STATS build &&\
  make CONF=Release.GENERIC.NO_STATS.NO_TLS install-examples clone-luajit clone-libressl

fi
