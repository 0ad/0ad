#!/bin/sh
set -e

OS="${OS:=$(uname -s)}"
# Apply patches if needed
# This script gets called from build.sh.

# The rust code is only linked if the JS Shell is enabled,
# which fails now that rust is required in all cases.
# https://bugzilla.mozilla.org/show_bug.cgi?id=1588340
patch -p1 < ../FixRustLinkage.diff

# On windows, we need to differente debug/release library names.
patch -p1 < ../FixWindowsLibNames.diff

# There is an issue on 32-bit linux builds sometimes.
# NB: the patch here is Comment 21 modified by Comment 25
# but that seems to imperfectly fix the issue with GCC.
# It also won't compile on windows - in doubt, apply only where relevant.
# https://bugzilla.mozilla.org/show_bug.cgi?id=1729459
if [ "$(uname -m)" = "i686" ] && [ "${OS}" != "Windows_NT" ]
then
	patch -p1 < ../FixFpNormIssue.diff
fi

if [ "$OS" = "Darwin" ]
then
    # The bundled virtualenv version is not working on MacOS
    # with recent homebrew and needs to be upgraded.
    # Install it locally to not pollute anything.
	pip3 install --upgrade -t virtualenv virtualenv
    export PYTHONPATH="$(pwd)/virtualenv:$PYTHONPATH"
    patch -p1 < ../FixVirtualEnv.diff
fi
