# Checkrefs.pl

## Description

This script checks the game files for missing dependencies, unused files, and for file integrity. If mods are specified, all their dependencies are also checked recursively. This script is particularly useful to detect broken actors or templates.

## Requirements

- Perl interpreter installed
- Dependencies:
  - XML::Parser
  - XML::Simple
  - Getopt::Long
  - File::Find
  - Data::Dumper
  - JSON

## Usage

- cd in source/tools/entity and run the script.

```
Usage: perl checkrefs.pl [OPTION]...
Checks the game files for missing dependencies, unused files, and for file integrity.
      --check-unused         check for all the unused files in the given mods and their dependencies. Implies --check-map-xml. Currently yields a lot of false positives.
      --check-map-xml        check maps for missing actor and templates.
      --validate-templates   run the validate.pl script to check if the xml files match their (.rng) grammar file. This currently only works for the public mod.
      --mod-to-check=mods    specify which mods to check. 'mods' should be a list of mods separated by '|'. Default value: 'public|mod'.
```
