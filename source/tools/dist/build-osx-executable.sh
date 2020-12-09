#!/bin/sh

# Build the Pyrogenesis executable, used to create the bundle and run the archiver.

# TODO: is there anything to do for ARM support?
export ARCH=${ARCH:="x86_64"}

# Set mimimum required OS X version, SDK location and tools
# Old SDKs can be found at https://github.com/phracker/MacOSX-SDKs
export MIN_OSX_VERSION=${MIN_OSX_VERSION:="10.12"}
# Note that the 10.12 SDK is know to be too old for FMT 7.
export SYSROOT=${SYSROOT:="/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk"}
export CC=${CC:="clang"} CXX=${CXX:="clang++"}

die()
{
  echo ERROR: $*
  exit 1
}

# Check that we're actually on OS X
if [ "`uname -s`" != "Darwin" ]; then
  die "This script is intended for OS X only"
fi

# Check SDK exists
if [ ! -d "$SYSROOT" ]; then
  die "$SYSROOT does not exist! You probably need to install Xcode"
fi

# Assume this is called from trunk/
cd "build/workspaces/"

JOBS=${JOBS:="-j5"}

# TODO: Do we really want to regenerate everything? (consider if one task fails)

./clean-workspaces.sh

# Build libraries against SDK
echo "\nBuilding libraries\n"
pushd ../../libraries/osx > /dev/null
./build-osx-libs.sh $JOBS --force-rebuild || die "Libraries build script failed"
popd > /dev/null

# Update workspaces
echo "\nGenerating workspaces\n"

# Pass OS X options through to Premake
(SYSROOT="$SYSROOT" MIN_OSX_VERSION="$MIN_OSX_VERSION" ./update-workspaces.sh --sysroot="$SYSROOT" --macosx-version-min="$MIN_OSX_VERSION") || die "update-workspaces.sh failed!"

pushd gcc > /dev/null
echo "\nBuilding game\n"
(make clean && CC="$CC -arch $ARCH" CXX="$CXX -arch $ARCH" make ${JOBS}) || die "Game build failed!"
popd > /dev/null

# Run test to confirm all is OK
pushd ../../binaries/system > /dev/null
echo "\nRunning tests\n"
./test || die "Post-build testing failed!"
popd > /dev/null
