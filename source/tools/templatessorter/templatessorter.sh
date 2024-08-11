#!/bin/bash
# check arguments count
if [ $# -ne 1 ]; then
  echo 'usage: '$0' directory'
  exit
fi
# assign arguments to variables with readable names
input_directory=$1
# perform work
find $input_directory -name \*.xml -exec xsltproc -o {} templatessorter.xsl {} \;

