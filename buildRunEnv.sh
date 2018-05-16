#!/bin/bash

docker build -t lapps:runenv -f dockerfiles/Dockerfile.lapps-runenv.0.6.3 --force-rm  .
