#!/bin/sh

cd premake

./premake --target gnu

mkdir -p ../gcc
cd ../gcc
mv -f ../premake/Makefile ../premake/prometheus.make .

# These files need to be linked; premake makefiles assume that the
# lua file is accessible from the makefile directory

ln -f -s ../premake/premake.lua ../premake/functions.lua .
if [ -x ../premake/premake ]; then
	ln -f -s ../premake/premake .
fi

cd ..
