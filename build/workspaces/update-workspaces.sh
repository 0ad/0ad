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
    * ) premake_args="${premake_args} $i"
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

# Make sure workspaces/gcc exists.
mkdir -p gcc

# Now build premake and run it to create the makefiles
cd ../premake
make -C src ${JOBS} || die "Premake build failed"

echo

# If we're in bash then make HOSTTYPE available to Premake, for primitive arch-detection
export HOSTTYPE="$HOSTTYPE"

src/bin/premake --outpath ../workspaces/gcc ${premake_args} --target gnu || die "Premake failed"
