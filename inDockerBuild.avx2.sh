#!/bin/bash

export VERSION=$(cat VERSION)
if [ "${VERSION}x" != "x" ]
then

  (cd ../ITCLib && git pull) && (cd ../ITCFramework && git pull) && \
  (cd ../utils && git pull) && (cd ../lar && git pull && make install) && git pull &&\
  make CONF=Release.AVX2 clean build &&\
  make CONF=Release.AVX2.NO_STATS build &&\
  make CONF=Release.AVX2.NO_STATS.NO_TLS build &&\
  make CONF=Release.AVX2.NO_STATS.NO_TLS install-examples clone-luajit clone-libressl

fi
