#!/bin/bash

export VERSION=$(cat VERSION)
if [ "${VERSION}x" != "x" ]
then

  rm -f /opt/lapps/bin/lapps* 
  (cd ../ITCLib && git pull) && (cd ../ITCFramework && git pull) && \
  (cd ../utils && git pull) && (cd ../lar && git pull && make install)  && git pull &&\
  make CONF=Release.GENERIC clean &&\
  make CONF=Release.GENERIC build install &&\
  make CONF=Release.GENERIC.NO_STATS build install &&\
  make CONF=Release.GENERIC.NO_STATS.NO_TLS build install install-examples clone-luajit clone-libressl

  ls -lrt /opt/lapps/bin

fi
