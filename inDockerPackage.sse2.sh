#!/bin/bash

export VERSION=$(cat VERSION)

./inDockerBuild.sse2.sh && rm -f /opt/lapps/bin/lapps.avx* /opt/lapps/bin/lapps.ssse3* && make CONF=Release.SSE2 build-deb && mv /opt/distrib/lapps-${VERSION}-amd64.deb /opt/lapps/packages/lapps-${VERSION}-sse2-amd64.deb
