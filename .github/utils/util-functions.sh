#! /bin/bash

RED=$(echo -e "\e[0;31m")
GREEN=$(echo -e "\e[0;32m")
YELLOW=$(echo -e "\e[0;33m")
BLUE=$(echo -e "\e[0;34m")
RESETCOLOR=$(echo -e "\e[0m")

#-------------------------------------------------------------------------------
# Log functions
#-------------------------------------------------------------------------------

print_section() {
  local sep40="----------------------------------------"
  printf '\n\n%s\n%s\n%s\n\n' "$sep40$sep40" " $*" "$sep40$sep40"
}

print_list_no_split() {
  # This function will print strings in a list and not split on space
  for item in "$@"; do
    printf '%s\n' "* $item"
  done
  echo
}

print_list() {
  # This function will print each item separated by space

  # shellcheck disable=SC2068
  for item in $@; do
    printf '%s\n' "* $item"
  done
  echo
}

print_bullet() {
  printf '%s\n' "* $*"
}

print_line() {
  printf '%s\n' "$*"
}

#-------------------------------------------------------------------------------
# Color coded log functions
#-------------------------------------------------------------------------------

print_color() {
  local color=$RESETCOLOR
  local color_in=$1 method_in=$2

  # Need to shift arguments to only print content
  shift && shift

  case $color_in in
    blue) color=$BLUE ;;
    red) color=$RED ;;
    green) color=$GREEN ;;
    yellow) color=$YELLOW ;;
    *) print_line "Color '$color_in' not supported" ;;
  esac

  case $method_in in
    line) print_line "${color}$*${RESETCOLOR}" ;;
    bullet) print_bullet "${color}$*${RESETCOLOR}" ;;
    *) print_line "Method '$method_in' not supported" ;;
  esac
}

print_line_error() {
  print_color red line "$*"
  echo
}

print_bullet_error() {
  print_color red bullet "$*"
}

print_bullet_pass() {
  print_color green bullet "$*"
}

print_list_no_split_error() {
  # This function will print strings in a list and not split on space
  for item in "$@"; do
    printf '%s\n' "* ${RED}${item}${RESETCOLOR}"
  done
  echo
}

print_list_error() {
  # This function will print each item separated by space

  # shellcheck disable=SC2068
  for item in $@; do
    printf '%s\n' "* ${RED}${item}${RESETCOLOR}"
  done
  echo
}

print_linebreaked_list_error() {
  # This function will print each item separated by linebreak

  oldIFS=$IFS
  IFS=$'\n'
  # shellcheck disable=SC2068
  for item in $@; do
    printf '%s\n' "* ${RED}${item}${RESETCOLOR}"
  done
  IFS=$oldIFS
  echo
}
