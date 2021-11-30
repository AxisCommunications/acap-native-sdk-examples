#!/bin/bash -e

# Make sure a environment name was given
[ $# -eq 1 ] || {
    echo "Argument should be: <ENVIRONMENT_NAME>"
    exit 1
}

ENV_NAME=$1
# If the --no-gpu option is set, we should empty the GPU_FLAG to disable GPU
# acceleration in the container
GPU_FLAG='--gpus all'
for arg in "$@"; do
  shift
  case "$arg" in
    "--no-gpu") GPU_FLAG='' ;;
  esac
done

# shellcheck disable=SC2086 # Deliberately word split arguments
docker run $GPU_FLAG -v /var/run/docker.sock:/var/run/docker.sock --network host --name $ENV_NAME -it tensorflow-to-larod /bin/bash
