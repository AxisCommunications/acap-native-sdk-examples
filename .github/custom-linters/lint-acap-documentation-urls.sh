#!/bin/bash

. "$(pwd)"/.github/utils/util-functions.sh

WORKFLOWFILE=${0##*/}

#-------------------------------------------------------------------------------
# Functions
#-------------------------------------------------------------------------------

check_acap_ver_urls() {
  local ret=0
  local __found_doc_url=
  local search_urls="\
    help.axis.com/acap-3-developer-guide \
    www.axis.com/techsup/developer_doc \
    axiscommunications.github.io/acap-documentation"
  local correct_url="https://developer.axis.com/acap"

  print_section "Verify that no old ACAP documentation URLs found in docs"

  # Check for incorrect URLs
  for doc_url in $search_urls; do
    __found_doc_url=$(grep -niIr "$doc_url" --exclude "$WORKFLOWFILE")
    [ -z "$__found_doc_url" ] || {
      print_line "ERROR: Old doc URL '$doc_url' found."
      print_line "       New doc URL '$correct_url' should be used:"
      print_linebreaked_list_error "$__found_doc_url"
      ret=1
    }
  done

  [ "$__found_doc_url" ] || {
    print_bullet_pass "No old ACAP documentation URLs found."
  }

  return $ret
}

check_acap_doc_no_md_in_link() {
  local ret=0
  local search_urls="axiscommunications.github.io/acap-documentation.*\.md.*"

  print_section "Verify that the docs don't have links to ACAP documentation with .md"

  # Check for incorrect URLs
  for doc_url in $search_urls; do
    __found_doc_url=$(grep -niIrE "$doc_url" --exclude "$WORKFLOWFILE")
    [ -z "$__found_doc_url" ] || {
      print_line "ERROR: Found URL to ACAP documentation containing '.md', which gives broken link."
      print_line "       This is due to that ACAP documentation converts Markdown to HTML."
      print_line "       Search pattern: '$doc_url'"
      print_linebreaked_list_error "$__found_doc_url"
      ret=1
    }
  done

  [ "$__found_doc_url" ] || {
    print_bullet_pass "No ACAP documentation links found to contain '.md'."
  }

  return $ret
}

#-------------------------------------------------------------------------------
# Main
#-------------------------------------------------------------------------------

exit_value=0
found_error=no

if ! check_acap_ver_urls; then found_error=yes; fi
if ! check_acap_doc_no_md_in_link; then found_error=yes; fi

[ "$found_error" = no ] || exit_value=1

exit $exit_value
