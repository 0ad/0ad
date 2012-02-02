#!/bin/sh

die()
{
  echo ERROR: $*
  exit 1
}

JOBS=${JOBS:="-j2"}

# FreeBSD's make is different than GNU make, so we allow overriding the make command.
# If not set, MAKE will default to "gmake" on FreeBSD, or "make" on other OSes
if [ "`uname -s`" = "FreeBSD" ]
then
  MAKE=${MAKE:="gmake"}
else
  MAKE=${MAKE:="make"}
fi

# Parse command-line options:

premake_args=""

with_system_nvtt=false
with_system_enet=false
with_system_mozjs185=false
enable_atlas=true

for i in "$@"
do
  case $i in
    --with-system-nvtt ) with_system_nvtt=true; premake_args="${premake_args} --with-system-nvtt" ;;
    --with-system-enet ) with_system_enet=true; premake_args="${premake_args} --with-system-enet" ;;
    --with-system-mozjs185 ) with_system_mozjs185=true; premake_args="${premake_args} --with-system-mozjs185" ;;
    --enable-atlas ) enable_atlas=true ;;
    --disable-atlas ) enable_atlas=false ;;
    -j* ) JOBS=$i ;;
    # Assume any other --options are for Premake
    --* ) premake_args="${premake_args} $i" ;;
  esac
done

premake_args="${premake_args} --collada"
if [ "$enable_atlas" = "true" ]; then
  premake_args="${premake_args} --atlas"
fi


cd "$(dirname $0)"
# Now in build/workspaces/ (where we assume this script resides)

echo "Updating bundled third-party dependencies..."
echo

# Build/update bundled external libraries
(cd ../../libraries/fcollada/src && ${MAKE} ${JOBS}) || die "FCollada build failed"
echo
if [ "$with_system_mozjs185" = "false" ]; then
  (cd ../../libraries/spidermonkey && MAKE=${MAKE} JOBS=${JOBS} ./build.sh) || die "SpiderMonkey build failed"
fi
echo
if [ "$with_system_nvtt" = "false" ]; then
  (cd ../../libraries/nvtt && MAKE=${MAKE} JOBS=${JOBS} ./build.sh) || die "NVTT build failed"
fi
echo
if [ "$with_system_enet" = "false" ]; then
  (cd ../../libraries/enet && MAKE=${MAKE} JOBS=${JOBS} ./build.sh) || die "ENet build failed"
fi
echo

# Now build premake and run it to create the makefiles
cd ../premake/premake4
${MAKE} -C build/gmake.unix ${JOBS} || die "Premake build failed"

echo

cd ..

# If we're in bash then make HOSTTYPE available to Premake, for primitive arch-detection
export HOSTTYPE="$HOSTTYPE"

premake4/bin/release/premake4 --file="premake4.lua" --outpath="../workspaces/gcc/" ${premake_args} gmake || die "Premake failed"
premake4/bin/release/premake4 --file="premake4.lua" --outpath="../workspaces/codeblocks/" ${premake_args} codeblocks || die "Premake failed"

# Also generate xcode3 workspaces if on OS X
if [ "`uname -s`" = "Darwin" ]
then
  premake4/bin/release/premake4 --file="premake4.lua" --outpath="../workspaces/xcode3" ${premake_args} xcode3 || die "Premake failed"
fi
