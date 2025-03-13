#!/bin/bash

. "$(git rev-parse --show-toplevel)"/.github/utils/util-functions.sh

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
  local search_urls="\
    axiscommunications.github.io/acap-documentation.*\.md.* \
    github.com/AxisCommunications/developer-documentation/.*\.md.*
    https://developer.axis.com/.*\.md.*
    "

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

check_relative_links_in_repo() {
  local ret=0
  local search_url="github.com/AxisCommunications/acap-native-sdk-examples(-staging)?"
  local exclude_dir_list="\
    --exclude-dir=.github \
    --exclude-dir=.git \
    "
  print_section "Verify that links to other pages in this repo are not direct URLs"

  # shellcheck disable=SC2086
  __found_doc_url=$(grep -niIrE "$search_url" --exclude "$WORKFLOWFILE" $exclude_dir_list)
  [ -z "$__found_doc_url" ] || {
    print_line "ERROR: Found direct URLs to other pages in this repository."
    print_line "       Use relative paths for these cases, like [link](../../example)."
    print_line "       Search pattern: '$search_url'"
    print_linebreaked_list_error "$__found_doc_url"
    ret=1
  }

  [ "$__found_doc_url" ] || {
    print_bullet_pass "No links were found to be direct URLs to other pages in this repo."
  }

  return $ret
}

check_no_links_to_internal_axis_repos() {
  local ret=0
  local search_url="github.com/AxisCommunications/.*-(staging|stage|private|internal)"
  local exclude_dir_list="\
    --exclude-dir=.github \
    --exclude-dir=.git \
    "
  print_section "Verify that links don't point to internal Axis repos"

  # shellcheck disable=SC2086
  __found_doc_url=$(grep -niIrE "$search_url" --exclude "$WORKFLOWFILE" $exclude_dir_list)
  [ -z "$__found_doc_url" ] || {
    print_line "ERROR: Found URLs pointing to internal Axis repositories."
    print_line "       Refer to the public repository."
    print_line "       Search pattern: '$search_url'"
    print_linebreaked_list_error "$__found_doc_url"
    ret=1
  }

  [ "$__found_doc_url" ] || {
    print_bullet_pass "No links were found to point to internal repos."
  }

  return $ret
}

check_no_private_docker_url() {
  local ret=0
  local __docker_grep_res=
  local priv_url=hub.docker.com/repository/docker/
  local pub_url=hub.docker.com/r/

  print_section "Check that no private Docker Hub URLs are used"

  __docker_grep_res=$(grep -niIr "$priv_url" --exclude "$WORKFLOWFILE")

  if [ "$__docker_grep_res" ]; then
    print_line "ERROR: Found private Docker Hub URL."
    print_line "       Private URL format: $priv_url"
    print_line "       Public URL format: $pub_url"
    print_linebreaked_list_error "$__docker_grep_res"
    ret=1
  else
    print_bullet_pass "No private Docker Hub URLs found."
  fi
  return $ret
}
#-------------------------------------------------------------------------------
# Main
#-------------------------------------------------------------------------------

exit_value=0
found_error=no

if ! check_acap_ver_urls; then found_error=yes; fi
if ! check_acap_doc_no_md_in_link; then found_error=yes; fi
if ! check_relative_links_in_repo; then found_error=yes; fi
if ! check_no_links_to_internal_axis_repos; then found_error=yes; fi
if ! check_no_private_docker_url; then found_error=yes; fi

[ "$found_error" = no ] || exit_value=1

exit $exit_value
