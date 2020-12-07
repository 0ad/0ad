#!/bin/sh
set -e

## This script runs all necessary steps to make a bundle
## It is not used directly by the CI, which calls those steps independently.
## Assume we are being called from trunk/

./source/tools/dist/build-osx-executable.sh
./source/tools/dist/build-archives.sh
python3 source/tools/dist/build-osx-bundle.py
# Note that at this point, you'll have left-over compilation files.
# The CI cleans them via svn st | cut -c9- | xargs rm -rf
./source/tools/dist/build-unix-win32.sh
