#!/bin/sh

# build/workspaces/

start_dir=$(pwd)
premake_dir=$(pwd)/../premake
workspace_dir=$(pwd)/gcc

cd $premake_dir

# build/premake/

mkdir -p tmp
cp premake.lua tmp
cd tmp

# build/premake/tmp/
../premake --target gnu

mkdir -p $workspace_dir
mv -f Makefile pyrogenesis.make $workspace_dir

# These files need to be linked; premake makefiles assume that the
# lua file is accessible from the makefile directory

cd $workspace_dir
ln -f -s $premake_dir/premake.lua $premake_dir/functions.lua .
if [ -x $premake_dir/premake ]; then
	ln -f -s $premake_dir/premake .
fi

cd $start_dir
