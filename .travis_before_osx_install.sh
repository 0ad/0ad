#!/bin/bash

brew update
brew install  \
  enet libogg glew gloox libogg libsodium \
  libvorbis libxml2 miniupnpc sdl2

brew outdated cmake || brew upgrade cmake
brew outdated pkgconfig || brew upgrade pkgconfig
