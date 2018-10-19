#!/bin/bash

./inDockerBuild.sh && make build-deb && mv /opt/distrib/lapps-${VERSION}-avx2-amd64.deb /opt/lapps/packages/
