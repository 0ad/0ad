#!/bin/sh

die()
{
  echo ERROR: $*
  exit 1
}

JOBS=${JOBS:="-j2"}

# Parse command-line options:

premake_args=""

with_system_nvtt=false
with_system_enet=false
enable_atlas=true

if [ "`uname -s`" = "Darwin" ]
then
  # Atlas is broken on OS X so disable by default
  enable_atlas=false
fi

for i in "$@"
do
  case $i in
    --with-system-nvtt ) with_system_nvtt=true; premake_args="${premake_args} --with-system-nvtt" ;;
    --with-system-enet ) with_system_enet=true; premake_args="${premake_args} --with-system-enet" ;;
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
(cd ../../libraries/fcollada/src && make ${JOBS}) || die "FCollada build failed"
echo
(cd ../../libraries/spidermonkey && JOBS=${JOBS} ./build.sh) || die "SpiderMonkey build failed"
echo
if [ "$with_system_nvtt" = "false" ]; then
  (cd ../../libraries/nvtt && JOBS=${JOBS} ./build.sh) || die "NVTT build failed"
fi
echo
if [ "$with_system_enet" = "false" ]; then
  (cd ../../libraries/enet && JOBS=${JOBS} ./build.sh) || die "ENet build failed"
fi
echo

# Now build premake and run it to create the makefiles
cd ../premake/premake4
make -C build/gmake.unix ${JOBS} || die "Premake build failed"

echo

cd ..

# If we're in bash then make HOSTTYPE available to Premake, for primitive arch-detection
export HOSTTYPE="$HOSTTYPE"

premake4/bin/release/premake4 --file="premake4.lua" --outpath="../workspaces/gcc/" ${premake_args} gmake || die "Premake failed"

# Also generate xcode3 workspaces if on OS X
if [ "`uname -s`" = "Darwin" ]
then
  premake4/bin/release/premake4 --file="premake4.lua" --outpath="../workspaces/xcode3" ${premake_args} xcode3 || die "Premake failed"
fi
