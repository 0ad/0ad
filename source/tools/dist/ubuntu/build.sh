#!/bin/bash

set -e

REVISION=07970 # update all the relevant files after changing this

eval "$(gpg-agent --daemon)"

echo Make sure you updated the changelogs!

rm -rf build-0ad_lucid
rm -rf build-0ad-data

mkdir -p build-0ad_lucid
mkdir -p build-0ad-data

tar xzf 0ad-r${REVISION}-alpha-unix-build.tar.gz
cp 0ad-r${REVISION}-alpha-unix-build.tar.gz build-0ad_lucid/0ad_0.0.0+r${REVISION}.orig.tar.gz
mv 0ad-r${REVISION}-alpha build-0ad_lucid/0ad-0.0.0+r${REVISION}
pushd build-0ad_lucid/0ad-0.0.0+r${REVISION}
cp -r ../../debian-0ad debian
debuild -S
debuild -b
popd

tar xzf 0ad-r${REVISION}-alpha-unix-data.tar.gz
cp 0ad-r${REVISION}-alpha-unix-data.tar.gz build-0ad-data/0ad-data_0.0.0+r${REVISION}.orig.tar.gz
mv 0ad-r${REVISION}-alpha build-0ad-data/0ad-data-0.0.0+r${REVISION}
pushd build-0ad-data/0ad-data-0.0.0+r${REVISION}
cp -r ../../debian-0ad-data debian
debuild -S
debuild -b
popd

echo -e "To test install:\n  sudo dpkg -i build-0ad_lucid/0ad_0.0.0+r${REVISION}-1~wfgppa1~lucid1_i386.deb build-0ad-data/0ad-data_0.0.0+r${REVISION}-1~wfgppa1_all.deb"

# TODO: see https://help.launchpad.net/Packaging/PPA/Copying for copying to other release series
