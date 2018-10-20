#!/bin/bash

export VERSION=$(cat VERSION)

./inDockerBuild.generic.sh && rm -f /opt/lapps/bin/lapps.avx2* && make CONF=Release.GENERIC build-deb && mv /opt/distrib/lapps-${VERSION}-amd64.deb /opt/lapps/packages/lapps-${VERSION}-generic-amd64.deb
