#!/bin/bash

#-------------------------------------------------------------------------------
# Functions
#-------------------------------------------------------------------------------

usage() {
  echo "
--------------------------------------------------------------------------------
Utility script to demonstrate reproducibility of an application
--------------------------------------------------------------------------------
reproducible_packages.sh {build|test|clean} [aarch64|armv7hf]

Options:
  build       Build the application Docker images, copy application content to
              build directories and run tests
  test        Test reproducibility of the build artifacts
  clean       Clean the build artifacts; Docker images and build directories

Requirements:
  ACAP SDK 3.4 or higher

--------------------------------------------------------------------------------
"
}

build_and_extract() {
  # $1 architecture
  local __arch=$1

  # Run first build - without any reproducibility
  docker build --no-cache --build-arg ARCH="$__arch" --tag rep-"$__arch":1 .
  # shellcheck disable=SC2046 # Docker container ID never needs to have quotes
  docker cp $(docker create rep-"$__arch":1):/opt/app ./build1

  # Second build - with reproducibility
  docker build --no-cache --build-arg ARCH="$__arch" --build-arg TIMESTAMP="$(git log -1 --pretty=%ct)" --tag rep-"$__arch":2 .
  # shellcheck disable=SC2046 # Docker container ID never needs to have quotes
  docker cp $(docker create rep-"$__arch":2):/opt/app ./build2

  # Third build - with reproducibility
  docker build --no-cache --build-arg ARCH="$__arch" --build-arg TIMESTAMP="$(git log -1 --pretty=%ct)" --tag rep-"$__arch":3 .
  # shellcheck disable=SC2046 # Docker container ID never needs to have quotes
  docker cp $(docker create rep-"$__arch":3):/opt/app ./build3
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
  # $1 architecture
  local dockrepo=rep-$1
  local builddir=build

  for i in 1 2 3; do
    echo "Remove build direcory: ${builddir}$i"
    [ -d ${builddir}$i ] && rm -rf ${builddir}$i

    echo "Remove docker image: ${dockrepo}$i"
    # shellcheck disable=SC2086 # Docker container ID never needs to have quotes
    docker image rm -f ${dockrepo}:$i
  done
}

#-------------------------------------------------------------------------------
# Options
#-------------------------------------------------------------------------------

arch=
[ $# -eq 1 ] && arch=armv7hf
[ $# -eq 2 ] && arch=$2
[ $# -gt 2 ] && {
  usage
  echo "Error: One or two arguments allowed"
  exit 1
}
case $1 in
  build)
    build_and_extract "$arch"
    ;;
  test)
    check_reproducible_eap
    ;;
  clean)
    clean_reproducible_test_env "$arch"
    ;;
  *)
    usage
    exit 0
    ;;
esac
