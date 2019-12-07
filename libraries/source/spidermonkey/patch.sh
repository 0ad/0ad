#!/bin/sh
# Apply patches if needed
# This script gets called from build-osx-libs.sh and build.sh.

# Remove the unnecessary NSPR dependency.
# Will be included in SM52.
# https://bugzilla.mozilla.org/show_bug.cgi?id=1379539
patch -p1 < ../RemoveNSPRDependency.diff

# Fix the path to the moz.build file in the zlib module
patch -p1 < ../FixZLibMozBuild.diff

# === Fix the SM45 tracelogger ===
# This patch is a squashed version of several patches that were adapted
# to fix failing hunks.
#
# Applied in the following order, they are:
# * https://bugzilla.mozilla.org/show_bug.cgi?id=1266649
#    Handle failing to add to pointermap gracefully.
# * https://bugzilla.mozilla.org/show_bug.cgi?id=1280648
#    Don't cache based on pointers to movable GC things.
# * https://bugzilla.mozilla.org/show_bug.cgi?id=1255766
#    Also mark resizing of memory.
# * https://bugzilla.mozilla.org/show_bug.cgi?id=1259403
#    Only increase capacity by multiples of 2.
#    Always make sure there are 3 free slots for events.
# ===
patch -p1  < ../FixTracelogger.diff

# Patch embedded python psutil to work with FreeBSD 12 after revision 315662
# Based on: https://svnweb.freebsd.org/ports/head/sysutils/py-psutil121/files/patch-_psutil_bsd.c?revision=436575&view=markup
# psutil will be upgraded in SM60: https://bugzilla.mozilla.org/show_bug.cgi?id=1436857
patch -p0 < ../FixpsutilFreeBSD.diff

# Patch some parts of the code to support extra processor architectures
# Includes:
# * https://bugzilla.mozilla.org/show_bug.cgi?id=1143022 (for arm64)
# * https://bugzilla.mozilla.org/show_bug.cgi?id=1277742 (for aarch64)
# * https://bugzilla.mozilla.org/show_bug.cgi?id=1266366 (for ppc64)
patch -p1 < ../FixNonx86.diff

# Always link mozglue into the shared library when building standalone.
# Will be included in SM60. Custom version of the patch for SM45, which doesn't have the same build system.
# https://bugzilla.mozilla.org/show_bug.cgi?id=1176787
patch -p1 < ../FixMozglueStatic.diff

# JSPropertyDescriptor is not public in SM45.
# Will be fixed in SM52.
# https://bugzilla.mozilla.org/show_bug.cgi?id=1316079
patch -p1 < ../ExportJSPropertyDescriptor.diff

# When trying to link pyrogenesis, js::oom::GetThreadType() and js::ReportOutOfMemory()
# are marked as unresolved symbols.
# Will be included in SM52.
# https://bugzilla.mozilla.org/show_bug.cgi?id=1379538
patch -p1 < ../FixLinking.diff
