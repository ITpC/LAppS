#!/bin/bash

export VERSION=$(cat VERSION)

./inDockerBuild.generic.sh && make CONF=Release.GENERIC build-deb && mv /opt/distrib/lapps-${VERSION}-amd64.deb /opt/lapps/packages/lapps-${VERSION}-generic-amd64.deb
