#!/bin/bash

for file in `ls *.PMD | sed 's/.PMD//'`
do
  svn mv ${file}.PMD ${file}.pmd
done

