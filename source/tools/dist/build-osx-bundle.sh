#!/bin/sh
#
# This script will build an OS X app bundle for 0 A.D.
#
# App bundles are intended to be self-contained and portable.
# An SDK is required, usually included with Xcode. The SDK ensures
# that only those system libraries are used which are available on
# the chosen target and compatible systems.
#

# Force build architecture, as sometimes environment is broken.
# For a universal fat binary, the approach would be to build every
# library with both archs and combine them with lipo, then do the
# same thing with the game itself.
# Choices are "x86_64" or "i386" (ppc and ppc64 not supported)
export ARCH=${ARCH:="x86_64"}

# Set mimimum required OS X version, SDK location and tools
# Old SDKs can be found at https://github.com/phracker/MacOSX-SDKs
export MIN_OSX_VERSION=${MIN_OSX_VERSION:="10.9"}
export SYSROOT=${SYSROOT:="/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX${MIN_OSX_VERSION}.sdk"}
export CC=${CC:="clang"} CXX=${CXX:="clang++"}

# 0 A.D. release version
BUNDLE_VERSION=${BUNDLE_VERSION:="0.0.24dev"}
BUNDLE_FILENAME="0ad-${BUNDLE_VERSION}-alpha-osx64.dmg"

# Unique identifier string for this bundle (reverse-DNS style)
BUNDLE_IDENTIFIER=${BUNDLE_IDENTIFIER:="com.wildfiregames.0ad"}
# Minimum version of OS X on which the bundle will run
BUNDLE_MIN_OSX_VERSION="${MIN_OSX_VERSION}.0"

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

# We assume this script resides in source/tools/dist/
DMGBUILD_CONFIG="$(pwd)/dmgbuild-settings.py"
cd "$(dirname $0)/../../../build/workspaces/"

JOBS=${JOBS:="-j5"}
build_release=false

# Parse command-line options:
for i in "$@"
do
  case $i in
    -j* ) JOBS=$i ;;
    --release ) build_release=true ;;
  esac
done

# For release, export an SVN copy
if [ "$build_release" = "true" ]; then
  echo "\nExporting SVN and preparing release\n"

  SVN_REV=`svnversion -n ../..`
  rm -rf "0ad-export"
  svn export ../.. "0ad-export" || die "Error exporting SVN working directory"
  cd "0ad-export"
  rm -f binaries/data/config/dev.cfg
  # Only include translations for a subset of languages
  . source/tools/dist/remove-incomplete-translations.sh $build_path/binaries/data || die "Error excluding translations"
  echo L\"${SVN_REV}-release\" > build/svn_revision/svn_revision.txt
  cd build/workspaces
fi

BUNDLE_APP="0ad.app"
BUNDLE_DMG_NAME="0 A.D."
BUNDLE_OUTPUT=${BUNDLE_OUTPUT:="$(pwd)/${BUNDLE_APP}"}
BUNDLE_CONTENTS=$BUNDLE_OUTPUT/Contents
BUNDLE_BIN=$BUNDLE_CONTENTS/MacOS
BUNDLE_RESOURCES=$BUNDLE_CONTENTS/Resources
BUNDLE_FRAMEWORKS=$BUNDLE_CONTENTS/Frameworks
BUNDLE_PLUGINS=$BUNDLE_CONTENTS/PlugIns
BUNDLE_SHAREDSUPPORT=$BUNDLE_CONTENTS/SharedSupport

# TODO: Do we really want to regenerate everything? (consider if one task fails)

# Build libraries against SDK
echo "\nBuilding libraries\n"
pushd ../../libraries/osx > /dev/null
./build-osx-libs.sh $JOBS --force-rebuild || die "Libraries build script failed"
popd > /dev/null

# Clean and update workspaces
echo "\nGenerating workspaces\n"

# Pass OS X options through to Premake
(./clean-workspaces.sh && SYSROOT="$SYSROOT" MIN_OSX_VERSION="$MIN_OSX_VERSION" ./update-workspaces.sh --macosx-bundle="$BUNDLE_IDENTIFIER" --sysroot="$SYSROOT" --macosx-version-min="$MIN_OSX_VERSION") || die "update-workspaces.sh failed!"

pushd gcc > /dev/null
echo "\nBuilding game\n"
(make clean && CC="$CC -arch $ARCH" CXX="$CXX -arch $ARCH" make ${JOBS}) || die "Game build failed!"
popd > /dev/null

# Run test to confirm all is OK
pushd ../../binaries/system > /dev/null
echo "\nRunning tests\n"
./test || die "Post-build testing failed!"
popd > /dev/null

# Create bundle structure
echo "\nCreating bundle directories\n"
rm -rf "${BUNDLE_OUTPUT}"
mkdir -p "${BUNDLE_BIN}"
mkdir -p "${BUNDLE_FRAMEWORKS}"
mkdir -p "${BUNDLE_PLUGINS}"
mkdir -p "${BUNDLE_RESOURCES}"
mkdir -p "${BUNDLE_SHAREDSUPPORT}"

# Build archive(s) - don't archive the _test.* mods
pushd ../../binaries/data/mods > /dev/null
archives=""
for modname in [a-zA-Z0-9]*
do
  archives="${archives} ${modname}"
done
popd > /dev/null

pushd ../../binaries/system > /dev/null

for modname in $archives
do
  echo "\nBuilding archive for '${modname}'\n"
  ARCHIVEBUILD_INPUT="$(pwd)/../data/mods/${modname}"
  ARCHIVEBUILD_OUTPUT="${BUNDLE_RESOURCES}/data/mods/${modname}"

  # For some reason the output directory has to exist?
  mkdir -p "${ARCHIVEBUILD_OUTPUT}"

  (./pyrogenesis -archivebuild="${ARCHIVEBUILD_INPUT}" -archivebuild-output="${ARCHIVEBUILD_OUTPUT}/${modname}.zip") || die "Archive build for '${modname}' failed!"
done

# Copy binaries
echo "\nCopying binaries\n"
# Only pyrogenesis for now, until we find a way to load
#   multiple binaries from one app bundle
# TODO: Would be nicer if we could set this path in premake
cp pyrogenesis "${BUNDLE_BIN}"

# Copy libs
echo "\nCopying libs\n"
# TODO: Would be nicer if we could set this path in premake
cp -v libAtlasUI.dylib "${BUNDLE_FRAMEWORKS}"
cp -v libCollada.dylib "${BUNDLE_FRAMEWORKS}"

popd > /dev/null

# Copy data
echo "\nCopying non-archived game data\n"
pushd ../../binaries/data > /dev/null
if [ "$build_release" = "false" ]; then
  mv config/dev.cfg config/dev.bak
fi
cp -R -v config "${BUNDLE_RESOURCES}/data/"
cp -R -v l10n "${BUNDLE_RESOURCES}/data/"
cp -R -v tools "${BUNDLE_RESOURCES}/data/"
if [ "$build_release" = "false" ]; then
  mv config/dev.bak config/dev.cfg
fi
popd > /dev/null
cp -v ../resources/0ad.icns "${BUNDLE_RESOURCES}"
cp -v ../resources/InfoPlist.strings "${BUNDLE_RESOURCES}"

# Copy license/readmes
# TODO: Also want copies in the DMG - decide on layout
echo "\nCopying readmes\n"
cp -v ../../*.txt "${BUNDLE_RESOURCES}"
cp -v ../../libraries/LICENSE.txt "${BUNDLE_RESOURCES}/LIB_LICENSE.txt"

# Create Info.plist
echo "\nCreating Info.plist\n"
alias PlistBuddy=/usr/libexec/PlistBuddy
INFO_PLIST="${BUNDLE_CONTENTS}/Info.plist"

PlistBuddy -c "Add :CFBundleName string 0 A.D." "${INFO_PLIST}"
PlistBuddy -c "Add :CFBundleIdentifier string ${BUNDLE_IDENTIFIER}" "${INFO_PLIST}"
PlistBuddy -c "Add :CFBundleVersion string ${BUNDLE_VERSION}" "${INFO_PLIST}"
PlistBuddy -c "Add :CFBundlePackageType string APPL" "${INFO_PLIST}"
PlistBuddy -c "Add :CFBundleSignature string none" "${INFO_PLIST}"
PlistBuddy -c "Add :CFBundleExecutable string pyrogenesis" "${INFO_PLIST}"
PlistBuddy -c "Add :CFBundleShortVersionString string ${BUNDLE_VERSION}" "${INFO_PLIST}"
PlistBuddy -c "Add :CFBundleDevelopmentRegion string English" "${INFO_PLIST}"
PlistBuddy -c "Add :CFBundleInfoDictionaryVersion string 6.0" "${INFO_PLIST}"
PlistBuddy -c "Add :CFBundleIconFile string 0ad" "${INFO_PLIST}"
PlistBuddy -c "Add :LSHasLocalizedDisplayName bool true" "${INFO_PLIST}"
PlistBuddy -c "Add :LSMinimumSystemVersion string ${BUNDLE_MIN_OSX_VERSION}" "${INFO_PLIST}"
PlistBuddy -c "Add :NSHumanReadableCopyright string Copyright © $(date +%Y) Wildfire Games" "${INFO_PLIST}"

# Package the app into a dmg
dmgbuild \
  -s "${DMGBUILD_CONFIG}" \
  -D app="${BUNDLE_OUTPUT}" \
  -D background="../../build/resources/dmgbackground.png" \
  "${BUNDLE_DMG_NAME}" "${BUNDLE_FILENAME}"


echo "\nBundle complete! Located in ${BUNDLE_OUTPUT}, compressed as ${BUNDLE_FILENAME}."
