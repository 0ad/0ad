#!/bin/sh

# Some of our makefiles depend on GNU make, so we set some sane defaults if MAKE
# is not set.
case "`uname -s`" in
  "FreeBSD" | "OpenBSD" )
    MAKE=${MAKE:="gmake"}
    ;;
  * )
    MAKE=${MAKE:="make"}
    ;;
esac

# (We don't attempt to clean up every last file here - output in
# binaries/system/ will still be there, etc. This is mostly just
# to quickly fix problems in the bundled dependencies.)

cd "$(dirname $0)"
# Now in build/workspaces/ (where we assume this script resides)

echo "Cleaning bundled third-party dependencies..."

(cd ../../libraries/fcollada/src && rm -rf ./output)
(cd ../../libraries/spidermonkey && rm -f .already-built)
(cd ../../libraries/spidermonkey && rm -rf ./js-1.8.5)
(cd ../../libraries/nvtt/src && rm -rf ./build)

(cd ../premake/premake4/build/gmake.bsd && ${MAKE} clean)
(cd ../premake/premake4/build/gmake.macosx && ${MAKE} clean)
(cd ../premake/premake4/build/gmake.unix && ${MAKE} clean)

echo "Cleaning build output..."

# Remove workspaces/gcc if present
rm -rf ./gcc
# Remove workspaces/codeblocks if present
rm -rf ./codeblocks
# Remove workspaces/xcode3 if present
rm -rf ./xcode3
# Remove workspaces/xcode4 if present
rm -rf ./xcode4

echo
echo "Done. Try running update-workspaces.sh again now."
