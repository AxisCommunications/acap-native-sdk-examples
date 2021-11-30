#!/bin/bash -e

docker build --build-arg http_proxy="${http_proxy:-}" \
             --build-arg https_proxy="${https_proxy:-}" \
             --tag tensorflow-to-larod .
