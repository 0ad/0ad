#!/bin/sh

# build/workspaces/

start_dir=$(pwd)
premake_dir=$(pwd)/../premake
workspace_dir=$(pwd)/gcc
mkdir "$workspace_dir" 2>/dev/null

cd "$premake_dir"

# build/premake/

make -C src
HOSTTYPE=$HOSTTYPE ./premake --outpath "$workspace_dir" --atlas --collada "$@" --target gnu

# These files need to be linked; premake makefiles assume that the
# lua file is accessible from the makefile directory

cd "$workspace_dir"
ln -f -s "$premake_dir"/premake.lua "$premake_dir"/functions.lua .
if [ -x "$premake_dir"/premake ]; then
    ln -f -s "$premake_dir"/premake .
fi

cd "$start_dir"
