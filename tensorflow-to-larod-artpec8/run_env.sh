#!/bin/sh -e

# Make sure a environment name was given
[ $# -eq 1 ] || [ $# -eq 2 ] || [ $# -eq 3 ] || {
    echo "Argument should be: <ENVIRONMENT_NAME> [--no-gpu] [--tag=<ENV_IMAGE>]"
    exit 1
}

ENV_NAME=$1
# If the --no-gpu option is set, we should empty the GPU_FLAG to disable GPU
# acceleration in the container
GPU_FLAG='--gpus all'
TAG=tensorflow-to-larod-a8
for arg in "$@"; do
  shift
  case "$arg" in
    --no-gpu) GPU_FLAG='' ;;
    --tag=*) TAG=${arg##*=} ;;
  esac
done

# shellcheck disable=SC2086 # Deliberately word split arguments
docker run $GPU_FLAG -v /var/run/docker.sock:/var/run/docker.sock --network host --name $ENV_NAME -it $TAG /bin/bash
