# Checkrefs.py

## Description

This script checks the game files for missing dependencies, unused files, and for file integrity. If mods are specified, all their dependencies are also checked recursively. This script is particularly useful to detect broken actors or templates.

## Requirements

- Python 3.6+ interpreter installed
- lxml for the -a option.

## Usage

- cd in `source/tools/entity` and run the script.

```
usage: checkrefs.py [-h] [-u] [-x] [-a] [-t] [-m MOD [MOD ...]]

Checks the game files for missing dependencies, unused files, and for file integrity.

options:
  -h, --help            show this help message and exit
  -u, --check-unused    check for all the unused files in the given mods and their dependencies. Implies --check-map-
                        xml. Currently yields a lot of false positives.
  -x, --check-map-xml   check maps for missing actor and templates.
  -a, --validate-actors
                        run the validator.py script to check if the actors files have extra or missing textures.
  -t, --validate-templates
                        run the validator.py script to check if the xml files match their (.rng) grammar file.
  -m MOD [MOD ...], --mods MOD [MOD ...]
                        specify which mods to check. Default to public.
```
