#!/bin/bash

docker build -t lapps:build -f dockerfiles/Dockerfile --force-rm .

