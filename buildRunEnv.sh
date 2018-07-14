#!/bin/bash

docker build -t lapps:runenv -f dockerfiles/Dockerfile.lapps-runenv.0.7.0 --force-rm  .
