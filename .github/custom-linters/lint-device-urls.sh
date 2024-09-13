#!/bin/bash

. "$(pwd)"/.github/utils/util-functions.sh

#-------------------------------------------------------------------------------
# Functions
#-------------------------------------------------------------------------------

check_device_ip_naming() {
  local ret=0
  local __url_grep_list=
  local allowed_name="AXIS_DEVICE_IP"
  local search_term="device"
  local disallowed_names_specials="DEVICE(-|_)(IP|URL)"
  local disallowed_names="\
    (axis-|axis_|)device(-|_)(ip|url) \
    <DEVICE(-|_)(IP|URL) \
    /DEVICE(-|_)(IP|URL) \
    "

  local exclude_dir_list="\
    --exclude-dir=.github \
    --exclude-dir=.git \
    --exclude-dir=build* \
    --exclude=alpine.tar \
    --exclude=.eap-install.cfg \
    "

  print_section "Check device IP names on format '$allowed_name'"

  for pattern in $disallowed_names; do
    # shellcheck disable=SC2086
    pattern_found=$(grep -niIrE "$search_term" $exclude_dir_list |
      grep -E "$pattern" || :)
    [ "$pattern_found" ] && __url_grep_list=$(printf '%s\n%s' "$pattern_found" "$__url_grep_list")
  done

  for pattern in $disallowed_names_specials; do
    # shellcheck disable=SC2086
    pattern_found=$(grep -niIrE "$search_term" $exclude_dir_list |
      grep -E "(^.*:[0-9]+:| )$pattern" || :)
    [ "$pattern_found" ] && __url_grep_list=$(printf '%s\n%s' "$pattern_found" "$__url_grep_list")
  done

  if [ "$__url_grep_list" ]; then
    print_line "ERROR: The following names for device IP are not allowed:"
    print_linebreaked_list_error "$__url_grep_list"

    print_line "Disallowed name patterns:"
    for pattern in $disallowed_names_specials; do
      printf '%s\n%s\n' "* <space>$pattern" "* ^$pattern"
    done
    print_list "$disallowed_names"

    print_line "Allowed name pattern (Including URLs prefixed http://):"
    print_list "$allowed_name"
    ret=1
  else
    print_bullet_pass "All device IPs found follow allowed naming"
  fi

  return $ret
}

check_device_ip_paths() {
  local ret=0
  local __url_grep_list=
  local base_url="<AXIS_DEVICE_IP>"
  local exclude_dir_list="\
    --exclude-dir=.github \
    --exclude-dir=.git \
    --exclude-dir=build* \
  "
  local allowed_patterns="\
    \`$base_url\` \
    $base_url<end-of-line> \
    $base_url<space> \
    $base_url: \
    $base_url/index.html#[a-z] \
    $base_url/axis-cgi \
    $base_url/local \
    @$base_url \
  "

  print_section "Check that device URLs follow allowed pattern"

  # shellcheck disable=SC2086
  __url_grep_list=$(grep -nIir "$base_url" $exclude_dir_list |
    grep -vE "$base_url(\`|$| |:|/axis-cgi|/index.html#[a-z]|/local)" |
    grep -vE "@$base_url" || :)

  if [ "$__url_grep_list" ]; then
    print_line "ERROR: The following device URLs are not matching allowed patterns:"
    print_linebreaked_list_error "$__url_grep_list"

    print_line "Allowed patterns (Including URLs prefixed http://):"
    print_list "$allowed_patterns"

    print_line "A typical error is to copy the redirected URL that include e.g. product specific string 'camera'"
    ret=1
  else
    print_bullet_pass "All device URLs follow pattern"
  fi

  return $ret
}

#-------------------------------------------------------------------------------
# Main
#-------------------------------------------------------------------------------

exit_value=0
found_error=no

if ! check_device_ip_naming; then found_error=yes; fi
if ! check_device_ip_paths; then found_error=yes; fi

[ "$found_error" = no ] || exit_value=1

exit $exit_value
