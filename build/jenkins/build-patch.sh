#!/bin/sh

set -e

# This script allows Jenkins to build and test a patch inside the phabricator
# workspace.

# The important thing is determining how much cleanup is needed. For instance,
# if a patch adds a source file, it is necessary to delete it afterwards and
# remove all related build artifacts. It will also be necessary to rebuild the game
# for testing the next patch, even if that new patch doesn't change the code.

# However, rebuilding everything each time is expensive and prevents us from
# handling a decent input of patches on Phabricator.

# This script does its best to determine a somewhat minimal necessary amount of
# cleanup, and resorts to a clean checkout in case of failure.

if [ -z "$DIFF_ID" ]; then
  echo 'No patch ID provided, aborting'
  exit 1
fi

# Build environment
JOBS=${JOBS:="-j2"}

# Move to the root of the repository (this script is in build/jenkins/)
cd "$(dirname $0)"/../../

# Utility functions

fully_cleaned=false

patch()
{
  # Revert previously applied changes or deleted files (see full_cleanup).
  svn revert -R .

  # Removing non-ignored files from binaries/data/ might be necessary and
  # doesn't impact the build, so do it each time.
  svn status binaries/data/ | cut -c 9- | xargs rm -rf

  # If we have non-ignored files under source/, we must delete them and
  # clean the workspaces.
  if [ "$(svn status source/)" ]; then
    svn status source/ | cut -c 9- | xargs rm -rf
    build/workspaces/clean-workspaces.sh --preserve-libs
  fi

  # Apply the patch
  # Patch errors (including successful hunks) are written to the Phab comment.
  # If patching is successful, this will be overwritten in the build() step.
  # If patching fails, the comment will be explicit about it.
  arc patch --diff "$DIFF_ID" --force 2> .phabricator-comment
}

full_cleanup()
{
  fully_cleaned=true
  svn st --no-ignore | cut -c 9- | xargs rm -rf
  patch # patch will revert everything, so don't worry about deleted files
}

build()
{
  {
    echo 'Updating workspaces...'
    build/workspaces/update-workspaces.sh "${JOBS}" --premake4 --with-system-nvtt --without-miniupnpc >/dev/null || echo 'Updating workspaces failed!'
    cd build/workspaces/gcc
    echo 'Build (release)...'
    make "${JOBS}" config=release 2>&1 1>/dev/null
    echo 'Build (debug)...'
    make "${JOBS}" config=debug 2>&1 1>/dev/null
    cd ../../../binaries/system
    echo 'Running release tests...'
    ./test 2>&1
    echo 'Running debug tests...'
    ./test_dbg 2>&1
    cd ../../source/tools/xmlvalidator
    echo 'Checking XML files...'
    perl validate.pl 2>&1 1>/dev/null
  } > .phabricator-comment
}


# Try to patch, if that manages to fail, clean up everything.
# If it patches, make sure the libraries are not touched.
if ! patch; then
  full_cleanup
elif svn status libraries/source/ | grep -v '^?' >/dev/null 2>&1; then
  full_cleanup
fi

# Try to build, if that fails, clean up everything.
if ! build && [ "$fully_cleaned" != true ]; then
  full_cleanup
  build
fi
