#!/bin/bash

brew update
brew outdated cmake || brew upgrade cmake; brew outdated pkgconfig || brew upgrade pkgconfig

cd libraries/osx
./build-osx-libs.sh

# Change back to src root. The commands to build are in .travis.yml
cd ../..
