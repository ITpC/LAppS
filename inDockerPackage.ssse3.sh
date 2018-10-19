#!/bin/bash

./inDockerBuild.ssse3.sh && make build-deb && mv /opt/distrib/lapps-${VERSION}-ssse3-amd64.deb /opt/lapps/packages/
