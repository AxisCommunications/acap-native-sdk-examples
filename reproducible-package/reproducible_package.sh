#!/bin/bash

#-------------------------------------------------------------------------------
# Functions
#-------------------------------------------------------------------------------

usage() {
  echo "
--------------------------------------------------------------------------------
Utility script to demonstrate reproducibility of an application
--------------------------------------------------------------------------------
reproducible_packages.sh [build|test|clean]

Options:
  build   Build the application Docker images, copy application content to build
          directories and run tests
  test    Test reproducibility of the build artifacts
  clean   Clean the build artifacts; Docker images and build directories

Requirements:
  ACAP SDK 3.4 or higher

--------------------------------------------------------------------------------
"
}

build_and_extract() {
  # Run first build - without any reproducibility
  docker build --no-cache --tag rep:1 .
  docker cp $(docker create rep:1):/opt/app ./build1

  # Second build - with reproducibility
  docker build --no-cache --build-arg TIMESTAMP="$(git log -1 --pretty=%ct)" --tag rep:2 .
  docker cp $(docker create rep:2):/opt/app ./build2

  # Third build - with reproducibility
  docker build --no-cache --build-arg TIMESTAMP="$(git log -1 --pretty=%ct)" --tag rep:3 .
  docker cp $(docker create rep:3):/opt/app ./build3
}

check_reproducible_eap() {
  local first_cmp="cmp build1/*.eap build2/*.eap"
  local second_cmp="cmp build2/*.eap build3/*.eap"

  printf "# Compare build 1 and 2 - diff expected\nCommand: $first_cmp\n"
  eval $first_cmp

  echo

  printf "# Compare build 2 and 3 - no diff expected\nCommand: $second_cmp\n"
  eval $second_cmp
}

clean_reproducible_test_env() {
  local dockrepo=rep
  local builddir=build

  for i in 1 2 3
  do
    printf "Remove build direcory: ${builddir}$i\n"
    [ -d ${builddir}$i ] && rm -rf ${builddir}$i

    printf "Remove docker image: ${dockrepo}$i\n"
    docker image rm -f ${dockrepo}:$i
  done
}


#-------------------------------------------------------------------------------
# Options
#-------------------------------------------------------------------------------

case $1 in
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

