#!/bin/bash

export VERSION=$(cat VERSION)

./inDockerBuild.ssse3.sh && rm -f /opt/lapps/bin/lapps.avx* /opt/lapps/bin/lapps.sse2* && make CONF=Release.SSSE3 build-deb && mv /opt/distrib/lapps-${VERSION}-amd64.deb /opt/lapps/packages/lapps-${VERSION}-ssse3-amd64.deb
