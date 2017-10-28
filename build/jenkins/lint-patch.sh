#!/bin/sh

# Lint errors should not count as build failures
set +e
set -v

# Move to the root of the repository (this script is in build/jenkins/)
cd "$(dirname $0)"/../../

arc patch --diff "$DIFF_ID" --force

svn st | grep '^[AM]' | cut -c 9- | xargs coala --ci --flush-cache --limit-files > .phabricator-comment
