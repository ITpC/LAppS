#!/bin/bash


docker run -it --rm --name runenv -h lapps --network=host -v /opt/lapps-builds/latest:/opt/lapps lapps:runenv bash
