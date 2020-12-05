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

# Fix bindgen trying to use different clang settings.
# https://bugzilla.mozilla.org/show_bug.cgi?id=1526857 (landed in SM69)
patch -p1 < ../FixBindgenClang.diff

# Fix gcc/clang macro extension
# https://bugzilla.mozilla.org/show_bug.cgi?id=1614243 (landed in SM75)
patch -p1 < ../StandardDbgMacro.diff

# Fix public export on MSVC (C2487)
# https://bugzilla.mozilla.org/show_bug.cgi?id=1614243
# (mentionned in the comments, no patch/commit found)
patch -p1 < ../FixPublicExport.diff
