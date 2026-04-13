#!/bin/bash

#-------------------------------------------------------------------------------
# Functions
#-------------------------------------------------------------------------------

usage() {
  echo "
--------------------------------------------------------------------------------
Utility script to demonstrate reproducibility of an application
--------------------------------------------------------------------------------
reproducible_packages.sh [options] {build|test|clean}

Commands:
  build       Build the application Docker images, copy application content to
              build directories and run tests
  test        Test reproducibility of the build artifacts
  clean       Clean the build artifacts; Docker images and build directories

Options:
  --arch <arch>             Target architecture (aarch64|armv7hf, default: armv7hf)
  --repo <name>             Docker registry repository
  --sdk <name>              SDK image name
  --sdk-version <version>   SDK version
  --ubuntu-version <version> Ubuntu version

Requirements:
  ACAP SDK 3.4 or higher

--------------------------------------------------------------------------------
"
}

build_and_extract() {
  local build_args=()
  [ -n "$arch" ] && build_args+=(--build-arg ARCH="$arch")
  [ -n "$repo" ] && build_args+=(--build-arg REPO="$repo")
  [ -n "$sdk" ] && build_args+=(--build-arg SDK="$sdk")
  [ -n "$sdk_version" ] && build_args+=(--build-arg VERSION="$sdk_version")
  [ -n "$ubuntu_version" ] && build_args+=(--build-arg UBUNTU_VERSION="$ubuntu_version")

  # Run first build - without any reproducibility
  docker build --platform=linux/amd64 --no-cache "${build_args[@]}" --tag rep-"$arch":1 .
  # shellcheck disable=SC2046 # Docker container ID never needs to have quotes
  docker cp $(docker create --platform=linux/amd64 rep-"$arch":1):/opt/app ./build1

  # Second build - with reproducibility
  docker build --platform=linux/amd64 --no-cache "${build_args[@]}" --build-arg TIMESTAMP="$(git log -1 --pretty=%ct)" --tag rep-"$arch":2 .
  # shellcheck disable=SC2046 # Docker container ID never needs to have quotes
  docker cp $(docker create --platform=linux/amd64 rep-"$arch":2):/opt/app ./build2

  # Third build - with reproducibility
  docker build --platform=linux/amd64 --no-cache "${build_args[@]}" --build-arg TIMESTAMP="$(git log -1 --pretty=%ct)" --tag rep-"$arch":3 .
  # shellcheck disable=SC2046 # Docker container ID never needs to have quotes
  docker cp $(docker create --platform=linux/amd64 rep-"$arch":3):/opt/app ./build3
}

check_reproducible_eap() {
  local first_cmp="cmp build1/*.eap build2/*.eap"
  local second_cmp="cmp build2/*.eap build3/*.eap"

  printf '# Compare build 1 and 2 - diff expected\nCommand: %s\n' "$first_cmp"
  eval "$first_cmp"

  echo

  printf '# Compare build 2 and 3 - no diff expected\nCommand: %s\n' "$second_cmp"
  eval "$second_cmp"
}

clean_reproducible_test_env() {
  local dockrepo=rep-$arch
  local builddir=build

  for i in 1 2 3; do
    echo "Remove build direcory: ${builddir}$i"
    [ -d "${builddir}$i" ] && rm -rf "${builddir}$i"

    echo "Remove docker image: ${dockrepo}$i"
    docker image rm -f "${dockrepo}:$i"
  done
}

#-------------------------------------------------------------------------------
# Options
#-------------------------------------------------------------------------------

arch=armv7hf
repo=
sdk=
sdk_version=
ubuntu_version=

while [[ $# -gt 0 ]]; do
  case $1 in
    --arch)
      arch="$2"
      shift 2
      ;;
    --repo)
      repo="$2"
      shift 2
      ;;
    --sdk)
      sdk="$2"
      shift 2
      ;;
    --sdk-version)
      sdk_version="$2"
      shift 2
      ;;
    --ubuntu-version)
      ubuntu_version="$2"
      shift 2
      ;;
    -h | --help)
      usage
      exit 0
      ;;
    -*)
      usage
      echo "Error: Unknown option $1"
      exit 1
      ;;
    *)
      break
      ;;
  esac
done

command=$1

if [ -z "$command" ]; then
  usage
  exit 1
fi

case $command in
  build)
    build_and_extract
    ;;
  test)
    check_reproducible_eap
    ;;
  clean)
    clean_reproducible_test_env
    ;;
  *)
    usage
    exit 0
    ;;
esac
