#!/bin/bash

export VERSION=$(cat VERSION)

./inDockerBuild.sh && make CONF=Release.AVX2 build-deb && mv /opt/distrib/lapps-${VERSION}-avx2-amd64.deb /opt/lapps/packages/
