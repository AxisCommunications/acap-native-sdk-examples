#!/bin/bash

if [ $# -lt 1 ]; then
    echo "Argument should be: <APP_IMAGE>"
    exit 1
fi

docker build --build-arg http_proxy="${http_proxy}" --build-arg https_proxy="${https_proxy}" --tag "$1" .

rm -rf build/
docker cp $(docker create "$1"):/opt/app ./build
