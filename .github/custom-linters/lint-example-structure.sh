#!/bin/bash

REPO_ROOT="$(git rev-parse --show-toplevel)"
. "$REPO_ROOT"/.github/utils/util-functions.sh
cd "$REPO_ROOT" || exit 1

#-------------------------------------------------------------------------------
# Functions
#-------------------------------------------------------------------------------

workflow_exists() {
  local example_name="$1"
  [ -f ".github/workflows/${example_name}.yml" ]
}

collect_examples_for_lint() {
  local collected=()

  for dir in *; do
    [ -d "$dir" ] || continue

    # If the top-level directory has a workflow, it is an example on its own.
    if workflow_exists "$dir"; then
      collected+=("$dir")
      continue
    fi

    # Otherwise treat it as a container folder and collect child examples.
    local found_child_examples=no
    for child in "$dir"/*; do
      [ -d "$child" ] || continue
      [ -f "$child/Dockerfile" ] || continue
      found_child_examples=yes
      collected+=("$(basename "$child")")
    done

    # If neither a workflow nor child examples are found, keep it as-is so
    # the lint can report that the directory lacks a workflow.
    [ "$found_child_examples" = yes ] || collected+=("$dir")
  done

  if [ "${#collected[@]}" -gt 0 ]; then
    printf '%s\n' "${collected[@]}" | sort -u
  fi
}

check_examples_have_workflow_file() {
  local ret=0
  local fail_list=()
  local yml_array=
  local example_array=

  print_section "Verify that all examples have a workflow .yml file"

  # shellcheck disable=SC2207
  yml_array=($(ls .github/workflows))
  example_array=()
  while IFS= read -r example; do
    [ -n "$example" ] && example_array+=("$example")
  done < <(collect_examples_for_lint)

  for example in "${example_array[@]}"; do
    local found_workflow_file=no
    for yml in "${yml_array[@]}"; do
      [ "$example.yml" = "$yml" ] && {
        found_workflow_file=yes
        break
      }
    done
    [ "$found_workflow_file" = yes ] || fail_list+=("$example")
  done

  if [ "${#fail_list[@]}" -ne 0 ]; then
    print_line "Example directories"
    print_list_no_split "${example_array[@]}"
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
  local example_list=
  local readme_examples=
  local unique=

  print_section "Verify that all examples have an entry in top-README"

  example_list="$(collect_examples_for_lint)"

  # Find lines starting with '* ['
  # The grep removes anything but '[*]', then remove entries with space
  # or capital letter and finally removes the brackets.
  readme_examples="$(sed -n '/- \[/p' README.md |
                     grep -oe '\[.*\]' |
                     grep -v '[A-Z ]' |
                     grep -oe '[0-9a-z-]*')"

  print_line "# Control that all example directories have an entry in README"
  unique=$(printf '%s\n%s' "${example_list}" "${readme_examples}" | sort | uniq -u)
  if [ "$unique" ]; then
    print_line "ERROR: Mismatch between example directories and entries in README.md."
    print_line "       The following items are only found in one of the two:"
    print_list_error "$unique"
    ret=1
  else
    print_bullet_pass "All example directories have entries in README.md"
  fi
  echo

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
