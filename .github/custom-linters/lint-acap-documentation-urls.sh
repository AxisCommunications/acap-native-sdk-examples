#!/bin/bash

. "$(pwd)"/.github/utils/util-functions.sh

WORKFLOWFILE=${0##*/}

#-------------------------------------------------------------------------------
# Functions
#-------------------------------------------------------------------------------

check_acap_ver_urls() {
  local ret=0

  # Version specific URL strings
  local acap3_type_urls="help.axis.com/acap-3-developer-guide \
    www.axis.com/techsup/developer_doc"
  local acap4_type_urls="axiscommunications.github.io/acap-documentation"

  # Decide which SDK version this repo is
  local search_urls=
  local incorrect_sdk_version=
  local repo=

  repo=$(git remote get-url origin)
  case $repo in
    *acap-native-sdk-examples*)
      search_urls=$acap3_type_urls
      incorrect_sdk_version=3
      correct_sdk_version=4
      ;;
    *acap3-examples*)
      search_urls=$acap4_type_urls
      incorrect_sdk_version=4
      correct_sdk_version=3
      ;;
  esac

  print_section "Verify that the docs don't have links to wrong SDK version format"

  # Check for incorrect URLs
  for doc_url in $search_urls; do
    __found_doc_url=$(grep -niIr "$doc_url" --exclude "$WORKFLOWFILE")
    [ -z "$__found_doc_url" ] || {
      print_line "ERROR: URL of ACAP version $incorrect_sdk_version found in ACAP version $correct_sdk_version."
      print_line "       URL '$doc_url' found in the following files:"
      print_linebreaked_list_error "$__found_doc_url"
      ret=1
    }
  done

  [ "$__found_doc_url" ] || {
    print_bullet_pass "All URLs follow ACAP $correct_sdk_version pattern"
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
