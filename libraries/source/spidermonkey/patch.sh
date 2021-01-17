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

# Fix public export on MSVC (C2487)
# https://bugzilla.mozilla.org/show_bug.cgi?id=1614243
# (mentionned in the comments, no patch/commit found)
patch -p1 < ../FixPublicExport.diff

# Fix Rooted<void*> not working on VS17
# https://bugzilla.mozilla.org/show_bug.cgi?id=1679736
# (Landed in 85)
patch -p1 < ../FixMSVCRootedVoid.diff

# Two SDK-related issues.
# -ftrivial-auto-var-init is clang 8,
# but apple-clang 10.0.0 (the maximum in 10.13)
# doesn't actually have it, so patch it out.
# Secondly, there is a 'max SDK version' in SM,
# which is set to 10.15.4 in SM78.
# Upstream has changed this to 10.11 at the moment,
# so this patches it to an arbitrarily high Mac OS 11
patch -p1 < ../FixMacBuild.diff

# Fix FP access breaking compilation on RPI3+
# https://bugzilla.mozilla.org/show_bug.cgi?id=1526653
# https://bugzilla.mozilla.org/show_bug.cgi?id=1536491 
patch -p1 < ../FixRpiUnalignedFpAccess.diff
