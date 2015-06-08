#!/bin/bash

VFSROOT=${1:-"../../../binaries/data/mods"}

for schema in $(find "$VFSROOT" -name '*.rnc')
do
	trang "$schema" "${schema/.rnc/.rng}"
done
