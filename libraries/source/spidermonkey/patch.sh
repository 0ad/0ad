#!/bin/sh
# Apply patches if needed
# This script gets called from build.sh.

# SM78 fails to create virtual envs on macs with python > 3.7
# Unfortunately, 3.7 is mostly unavailable on ARM macs.
# Therefore, replace the custom script with a more up-to-date version from pip
# if python is detected to be newer than 3.7.
if [ "$(uname -s)" = "Darwin" ];
then
    PYTHON_MINOR_VERSION="$(python3 -c 'import sys; print(sys.version_info.minor)')"
    if [ "$PYTHON_MINOR_VERSION" -gt 7 ];
    then
        # SM actually uses features from the full-fledged virtualenv package
        # and not just venv, so install it to be safe.
        # Install it locally to not pollute anything.
        pip3 install --upgrade -t virtualenv virtualenv
        export PYTHONPATH="$(pwd)/virtualenv:$PYTHONPATH"
        patch -p1 < ../FixVirtualEnv.diff
    fi
fi

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

# Bug 1684261 upstreamed from 78.8: https://hg.mozilla.org/releases/mozilla-esr78/rev/0e8f444683cb
# Note that this isn't quite the upstream patch to match our version.
patch -p1 < ../FixRust150.diff

# Patch those separately, as they might interfere with normal behaviour.
if [ "$(uname -s)" = "FreeBSD" ];
then
    # https://svnweb.freebsd.org/ports/head/lang/spidermonkey78/files/patch-js_moz.configure?view=log
    patch -p1 < ../FixFreeBSDReadlineDetection.diff
    # https://svnweb.freebsd.org/ports/head/lang/spidermonkey78/files/patch-third__party_rust_cc_.cargo-checksum.json?view=log
    patch -p1 < ../FixFreeBSDCargoChecksum.diff
    # https://svnweb.freebsd.org/ports/head/lang/spidermonkey78/files/patch-third__party_rust_cc_src_lib.rs?view=log
    patch -p1 < ../FixFreeBSDRustThirdPartyOSDetection.diff
fi
