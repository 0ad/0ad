#!/bin/sh
# Apply patches if needed
# This script gets called from build.sh.

# Mozglue symbols need to be linked against static builds.
# https://bugzilla.mozilla.org/show_bug.cgi?id=1588340
patch -p1 < ../FixMozglue.diff

# Update library names to have separate debug/release libraries.
patch -p1 < ../RenameLibs.diff

# Fix ~SharedArrayRawBufferRefs symbol not found.
# See https://bugzilla.mozilla.org/show_bug.cgi?id=1644600
# Many thanks to bellaz89 for finding this and reporting it
patch -p1 < ../FixSharedArray.diff
