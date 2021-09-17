#!/bin/bash
UBUNTU_VERSION=20.04

if [ $# -lt 1 ]; then
    echo "Argument should be: <APP_IMAGE>"
    exit 1
fi

docker build --build-arg http_proxy="${http_proxy}" --build-arg https_proxy="${https_proxy}" --tag "$1" .

rm -rf result/ build/
docker cp $(docker create "$1":latest):/build/src/ build
