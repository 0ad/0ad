#!/bin/bash

# the latest Ubuntu version running on Travis is trusty, and there are
# dependencies that don't meet the required version, so we'll
# get them from the 0ad PPA
sudo add-apt-repository -y ppa:wfg/0ad
sudo apt-get update

# using -q reduces the travis log output
sudo apt-get install -q -y \
    libsodium-dev libgloox-dev \
    libenet-dev libboost1.58-dev libboost-filesystem1.58-dev

# when trying to install libsdl2-dev, these dependencies do not get
# installed automatically.
# https://github.com/travis-ci/travis-ci/issues/9065
sudo apt-get install -q   \
    libegl1-mesa-dev libgles2-mesa-dev

# install the remaining dependencies
sudo apt-get install -q   \
    libcurl4-gnutls-dev libicu-dev    \
    libminiupnpc-dev libnspr4-dev libnvtt-dev libogg-dev libopenal-dev   \
    libpng-dev libsdl2-dev libvorbis-dev libwxgtk3.0-dev libxcursor-dev  \
    libxml2-dev zlib1g-dev
