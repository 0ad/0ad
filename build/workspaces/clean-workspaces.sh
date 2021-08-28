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
  (cd ../../libraries/source/fcollada && rm -f .already-built)
  (cd ../../libraries/source/nvtt/src && rm -rf ./build)
  (cd ../../libraries/source/nvtt && rm -f .already-built)
  (cd ../../libraries/source/spidermonkey && rm -f .already-built)
  (cd ../../libraries/source/spidermonkey && rm -rf ./lib/*.a && rm -rf ./lib/*.so)
  (cd ../../libraries/source/spidermonkey && rm -rf ./include-unix-debug)
  (cd ../../libraries/source/spidermonkey && rm -rf ./include-unix-release)
  (cd ../../libraries/source/spidermonkey && rm -rf ./mozjs-68.12.1)
fi

# Still delete the directory of previous SpiderMonkey versions to
# avoid wasting disk space if people clean workspaces after updating.
(cd ../../libraries/source/spidermonkey && rm -rf ./mozjs-62.9.1)
(cd ../../libraries/source/spidermonkey && rm -rf ./mozjs-52.9.1pre1)
(cd ../../libraries/source/spidermonkey && rm -rf ./mozjs-45.0.2)
(cd ../../libraries/source/spidermonkey && rm -rf ./mozjs-38.0.0)
(cd ../../libraries/source/spidermonkey && rm -rf ./mozjs-38.0.0)
(cd ../../libraries/source/spidermonkey && rm -rf ./mozjs31)
(cd ../../libraries/source/spidermonkey && rm -rf ./mozjs24)

# Delete former premake5 gmake.* directories
(cd ../premake/premake5/build && rm -rf ./gmake.bsd)
(cd ../premake/premake5/build && rm -rf ./gmake.macosx)
(cd ../premake/premake5/build && rm -rf ./gmake.unix)

# Cleanup current premake directories
(cd ../premake/premake5/build/gmake2.bsd && ${MAKE} clean)
(cd ../premake/premake5/build/gmake2.macosx && ${MAKE} clean)
(cd ../premake/premake5/build/gmake2.unix && ${MAKE} clean)

echo "Removing generated stub and test files..."

find ../../source -name "stub_*.cpp" -type f -exec rm {} \;
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
