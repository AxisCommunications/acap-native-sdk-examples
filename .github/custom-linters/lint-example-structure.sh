#!/bin/bash

. "$(git rev-parse --show-toplevel)"/.github/utils/util-functions.sh

#-------------------------------------------------------------------------------
# Functions
#-------------------------------------------------------------------------------

check_examples_have_workflow_file() {
  local ret=0
  local fail_list=()
  local yml_array=
  local acap_array=

  print_section "Verify that all examples have a workflow .yml file"

  # shellcheck disable=SC2207
  yml_array=($(ls .github/workflows))
  # shellcheck disable=SC2207,SC2035
  acap_array=($(ls -d */))

  for acap in "${acap_array[@]%?}"; do
    local found_workflow_file=no
    for yml in "${yml_array[@]}"; do
      [ "$acap.yml" = "$yml" ] && {
        found_workflow_file=yes
        break
      }
    done
    [ "$found_workflow_file" = yes ] || fail_list+=("$acap")
  done

  if [ "${#fail_list[@]}" -ne 0 ]; then
    print_line "Application directories"
    print_list_no_split "${acap_array[@]%?}"
    print_line "YML files"
    print_list_no_split "${yml_array[@]}"
    print_line "ERROR: Applications that dont't have .yml file under .github/workflows:"
    print_list_no_split_error "${fail_list[@]}"
    ret=1
  else
    print_bullet_pass "All applications have .yml file."
  fi

  return $ret
}

check_examples_have_top_readme_entry() {
  local ret=0
  local acap_list=
  local readme_examples=
  local unique=

  print_section "Verify that all examples have an entry in top-README"

  # Find directories that do not start with dot
  acap_list="$(find . -maxdepth 1 -type d -printf '%f\n' |
               grep -v '^[.]' |
               sort)"

  # Find lines starting with '* ['
  # The grep removes anything but '[*]', then remove entries with space
  # or capital letter and finally removes the brackets.
  readme_examples="$(sed -n '/- \[/p' README.md |
                     grep -oe '\[.*\]' |
                     grep -v '[A-Z ]' |
                     grep -oe '[0-9a-z-]*')"

  print_line "# Control that all example directories have an entry in README"
  unique=$(printf '%s\n%s' "${acap_list}" "${readme_examples}" | sort | uniq -u)
  if [ "$unique" ]; then
    print_line "ERROR: Mismatch between example directories and entries in README.md."
    print_line "       The following items are only found in one of the two:"
    print_list_error "$unique"
    ret=1
  else
    print_bullet_pass "All example directories have entries in README.md"
  fi
  echo

  print_line "# Control that the README entries are sorted alphabetically ascending"
  sorted_list="$(printf '%s' "$readme_examples" | sort)"
  if [ "$readme_examples" != "$sorted_list" ]; then
    print_line "ERROR: The list of example entries in README.md is not sorted alphabetically."
    print_line "       Left column has current order, right is how it should be sorted:"
    diff --color -y <(echo "$readme_examples") <(echo "$sorted_list")
    ret=1
  else
    print_bullet_pass "All examples are sorted in alphabetical order in README.md."
  fi

  return $ret
}

#-------------------------------------------------------------------------------
# Main
#-------------------------------------------------------------------------------

exit_value=0
found_error=no

if ! check_examples_have_workflow_file; then found_error=yes; fi
if ! check_examples_have_top_readme_entry; then found_error=yes; fi

[ "$found_error" = no ] || exit_value=1

exit $exit_value
