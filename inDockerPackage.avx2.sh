#!/bin/bash

export VERSION=$(cat VERSION)

./inDockerBuild.avx2.sh && rm -f /opt/lapps/bin/lapps.ss* && make CONF=Release.AVX2 build-deb && mv /opt/distrib/lapps-${VERSION}-amd64.deb /opt/lapps/packages/lapps-${VERSION}-avx2-amd64.deb
