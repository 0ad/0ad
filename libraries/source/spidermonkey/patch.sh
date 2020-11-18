#!/bin/sh
# Apply patches if needed
# This script gets called from build-osx-libs.sh and build.sh.

# Patch embedded python psutil to work with FreeBSD 12 after revision 315662
# Based on: https://svnweb.freebsd.org/ports/head/sysutils/py-psutil121/files/patch-_psutil_bsd.c?revision=436575&view=markup
# psutil will be upgraded in SM60: https://bugzilla.mozilla.org/show_bug.cgi?id=1436857
patch -p0 < ../FixpsutilFreeBSD.diff

# Static builds have a few issues.
# First, mozglue symbols need to be linked against. This is partly fixed in SM60+, party still there in SM78.
# (see https://bugzilla.mozilla.org/show_bug.cgi?id=1176787 ).
# Secondly, the SDK_LIBRARY flag isn't set. This is still an issue in SM78.
# https://bugzilla.mozilla.org/show_bug.cgi?id=1588340
patch -p1 < ../FixMozglue.diff

# Update library names to have separate debug/release libraries.
patch -p1 < ../RenameLibs.diff

# After https://bugzilla.mozilla.org/show_bug.cgi?id=1289934 landed in
# Firefox 51, MSVC compilation has been broken due to bug
# https://developercommunity.visualstudio.com/content/problem/25334/error-code-c2971-when-specifying-a-function-as-the.html
# (resolved in Visual Studio 2017). The following fix was applied
# to not drop compiler support and will be included in SM 60.
# https://bugzilla.mozilla.org/show_bug.cgi?id=1300925
patch -p1 < ../FixMSVCBuild.diff

# GCC 9 and 10 fail to build with -Werror=format, so disable it.
# https://bugs.gentoo.org/693556
patch -p1 < ../DisableGCC9WerrorFormat.diff

