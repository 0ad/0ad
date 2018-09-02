#!/bin/bash

brew update
brew outdated cmake || brew upgrade cmake; brew outdated pkgconfig || brew upgrade pkgconfig

cd libraries/osx
# Travis will terminate the build if the log exceeds 4MB
# so we'll send the output of the script to /dev/null
./build-osx-libs.sh &> /dev/null

# Change back to src root. The commands to build are in .travis.yml
cd ../..
