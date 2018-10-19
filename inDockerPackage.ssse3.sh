#!/bin/bash

export VERSION=$(cat VERSION)

./inDockerBuild.ssse3.sh && make CONF=Release.GENERIC build-deb && mv /opt/distrib/lapps-${VERSION}-amd64.deb /opt/lapps/packages/lapps-${VERSION}-ssse3-amd64.deb
