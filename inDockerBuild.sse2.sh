#!/bin/bash

export VERSION=$(cat VERSION)
if [ "${VERSION}x" != "x" ]
then

  rm -f /opt/lapps/bin/lapps*

  (cd ../ITCLib && git pull) && (cd ../ITCFramework && git pull) && \
  (cd ../utils && git pull) && (cd ../lar && git pull && make install) && git pull &&\
  make CONF=Release.SSE2 clean &&\
  make CONF=Release.SSE2 build install &&\
  make CONF=Release.SSE2.NO_STATS build install &&\
  make CONF=Release.SSE2.NO_STATS.NO_TLS build install &&\
  make CONF=Release.SSE2.NO_STATS.NO_TLS install-examples clone-usr-local

  ls -la /opt/lapps/bin

fi
