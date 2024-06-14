#!/bin/bash

#-------------------------------------------------------------------------------
# Functions
#-------------------------------------------------------------------------------

print_section() {
  local sep40="----------------------------------------"
  printf '\n\n%s\n%s\n%s\n' "$sep40$sep40" " $*" "$sep40$sep40"
}

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
    "

  print_section "Check device IP names on format '$allowed_name'"

  for pattern in $disallowed_names; do
    # shellcheck disable=SC2086
    pattern_found=$(grep -nirE "$search_term" $exclude_dir_list |
      grep -E "$pattern" || :)
    [ "$pattern_found" ] && __url_grep_list="$__url_grep_list\n$pattern_found"
  done

  for pattern in $disallowed_names_specials; do
    # shellcheck disable=SC2086
    pattern_found=$(grep -nirE "$search_term" $exclude_dir_list |
      grep -E "(^.*:[0-9]+:| )$pattern" || :)
    [ "$pattern_found" ] && __url_grep_list="$__url_grep_list\n$pattern_found"
  done

  if [ "$__url_grep_list" ]; then
    printf '\n%s\n%s\n' \
      "## Error - The following names for device IP are not allowed:" \
      "$__url_grep_list"
    printf '\n%s\n' "## Disallowed name patterns:"
    for pattern in $disallowed_names; do
      printf '%s\n' "* $pattern"
    done
    for pattern in $disallowed_names_specials; do
      printf '%s\n%s\n' "* <space>$pattern" "* ^$pattern"
    done
    printf '\n%s\n%s\n' "## Allowed name (Including URLs prefixed http://):" "* $allowed_name"
    ret=1
  else
    printf "* All device IPs found follow allowed naming\n"
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
    $base_url/index.html# \
    $base_url/axis-cgi \
    $base_url/local \
    @$base_url \
  "

  print_section "Check that device URLs follow allowed pattern"

  # shellcheck disable=SC2086
  __url_grep_list=$(grep -nir "$base_url" $exclude_dir_list |
    grep -vE "$base_url(\`|$| |:|/axis-cgi|/index.html#|/local)" |
    grep -vE "@$base_url" || :)

  if [ "$__url_grep_list" ]; then
    printf '\n%s\n%s\n\n' \
      "## Error - The following device URLs are not matching allowed patterns" \
      "$__url_grep_list"
    printf "## Allowed patterns (Including URLs prefixed http://):\n"
    for pattern in $allowed_patterns; do
      printf '%s\n' "* $pattern"
    done
    printf "\nA typical error is to copy the redirected URL that include e.g. product specific string 'camera'\n"
    ret=1
  else
    printf "* All device URLs follow pattern\n"
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
