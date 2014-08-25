#!/bin/sh

pyrogenesis=$(which pyrogenesis 2> /dev/null)
if [ -x "$pyrogenesis" ] ; then
  "$pyrogenesis" -mod=public "$@"
else
  echo "Error: pyrogenesis not found in ($PATH)"
  exit 1
fi
