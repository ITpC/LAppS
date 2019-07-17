#!/bin/bash

export VERSION=$(cat VERSION)

./inDockerBuild.avx.sh && rm -f /opt/lapps/bin/lapps.ss* && make CONF=Release.AVX2 build-deb && mv /opt/distrib/lapps-${VERSION}-amd64.deb /opt/lapps/packages/lapps-${VERSION}-avx-amd64.deb
