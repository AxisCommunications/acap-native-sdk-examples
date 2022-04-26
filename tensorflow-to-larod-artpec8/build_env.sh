#!/bin/sh -e

docker build --build-arg http_proxy="${http_proxy:-}" \
             --build-arg https_proxy="${https_proxy:-}" \
             --tag "${1:-tensorflow-to-larod-a8}" .
