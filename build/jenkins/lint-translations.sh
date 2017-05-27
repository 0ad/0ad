#!/bin/sh

# This script uses the Dennis PO(T) linter to find issues.
# See http://dennis.readthedocs.io/en/latest/index.html for
# installation instructions.

set +e # Lint everything without failing

# Move to the root of the repository (this script is in build/jenkins/)
cd "$(dirname $0)"/../../

# Configuration for the linter
# Ignore
# - W302: Translated string is identical to source string
parameters='--excluderules W302'

# Run lint and output to a file that will be posted on Phabricator
echo "Running Dennis..."
{
	echo "Linting templates..."
	echo "Engine"
	dennis-cmd lint ${parameters} binaries/data/l10n/*.pot
	echo "Mod mod"
	dennis-cmd lint ${parameters} binaries/data/mods/mod/l10n/*.pot
	echo "Public mod"
	dennis-cmd lint ${parameters} binaries/data/mods/public/l10n/*.pot

	echo "Linting translations..."
	echo "Engine"
	dennis-cmd lint ${parameters} binaries/data/l10n/*.po
	echo "Mod mod"
	dennis-cmd lint ${parameters} binaries/data/mods/mod/l10n/*.po
	echo "Public mod"
	dennis-cmd lint ${parameters} binaries/data/mods/public/l10n/*.po
} > .phabricator-comment
