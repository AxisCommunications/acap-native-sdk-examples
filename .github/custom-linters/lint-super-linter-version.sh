#!/bin/bash

. "$(git rev-parse --show-toplevel)"/.github/utils/util-functions.sh

#-------------------------------------------------------------------------------
# Functions
#-------------------------------------------------------------------------------

check_super_linter_version_aligned_in_all_places() {
  local ret=0
  local fail_list=
  local workflow_dir=.github/workflows
  local workflow_version=
  local local_version=
  local lines_with_super_linter_image=
  local super_linter_repo=super-linter/super-linter
  local super_linter_action_name="$super_linter_repo/slim@v"
  local super_linter_image_name="$super_linter_repo:slim-v"
  local exclude_list="--exclude-dir=.git"

  print_section "Verify that version of super-linter is aligned in workflow, linters and docs"

  workflow_file_entry="$(cd "$workflow_dir" || print_line "Error: Can't cd to $workflow_dir" && exit 1 |
                         grep -rnIE $super_linter_action_name $exclude_list)"
  workflow_version="$(echo "$workflow_file_entry" |
                      grep -oE '@v[0-9]+' |
                      grep -oE '[0-9]+')"
  lines_with_super_linter_image="$(grep -rnIE $super_linter_image_name $exclude_list)"
  while read -r line; do
    local_version="$(echo "$line" |
                     grep -oE '\-v[0-9]+' |
                     grep -oE '[0-9]+')"
    [ "$local_version" = "$workflow_version" ] || {
      fail_list=$(printf '%s\n%s' "$line" "$fail_list")
    }
  done <<< "$lines_with_super_linter_image"

  if [ "$fail_list" ]; then
    print_line "ERROR: Version of super-linter is '$workflow_version' in workflow file"
    print_line "       \"$workflow_file_entry\""
    print_line "       The following places need to align super-linter version to the workflow:"
    print_linebreaked_list_error "$fail_list"
    ret=1
  else
    print_bullet_pass "Version of super-linter is aligned in workflow, linters and docs"
  fi

  return $ret
}

#-------------------------------------------------------------------------------
# Main
#-------------------------------------------------------------------------------

exit_value=0
found_error=no

if ! check_super_linter_version_aligned_in_all_places; then found_error=yes; fi

[ "$found_error" = no ] || exit_value=1

exit $exit_value
