#!/bin/bash

./inDockerBuild.sh && ./inDockerBuild.ssse3.sh && make build-deb && mv /opt/distrib/lapps-${VERSION}-amd64.deb /opt/lapps/packages/
