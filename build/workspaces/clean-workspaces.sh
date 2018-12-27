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

# Check the preserve-libs CL option
preserve_libs=false

for i in "$@"; do
  case "$i" in
    --preserve-libs ) preserve_libs=true ;;
  esac
done

# (We don't attempt to clean up every last file here - output in
# binaries/system/ will still be there, etc. This is mostly just
# to quickly fix problems in the bundled dependencies.)

cd "$(dirname $0)"
# Now in build/workspaces/ (where we assume this script resides)

if [ "$preserve_libs" != "true" ]; then
  echo "Cleaning bundled third-party dependencies..."

  (cd ../../libraries/source/fcollada/src && rm -rf ./output)
  (cd ../../libraries/source/nvtt/src && rm -rf ./build)
  (cd ../../libraries/source/spidermonkey && rm -f .already-built)
  (cd ../../libraries/source/spidermonkey && rm -rf ./mozjs-38.0.0)
fi

# Still delete the directory of previous SpiderMonkey versions to
# avoid wasting disk space if people clean workspaces after updating.
(cd ../../libraries/source/spidermonkey && rm -rf ./mozjs31)
(cd ../../libraries/source/spidermonkey && rm -rf ./mozjs24)

(cd ../premake/premake5/build/gmake.bsd && ${MAKE} clean)
(cd ../premake/premake5/build/gmake.macosx && ${MAKE} clean)
(cd ../premake/premake5/build/gmake.unix && ${MAKE} clean)

echo "Removing generated test files..."

find ../../source -name "test_*.cpp" -type f -not -name "test_setup.cpp" -exec rm {} \;

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
