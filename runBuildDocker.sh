#!/bin/bash

docker run -it --rm --name myldev -h ldev -v /opt/lapps-builds/latest:/opt/lapps lapps:build bash

