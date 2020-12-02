#!/bin/sh

# Regenerates the POT files, downloads the latest translations from Transifex,
# and prepares the commit of the updated POT and PO files.

SCRIPT_PATH="`dirname \"$0\"`"


# POT Generation ##############################################################

echo ":: Regenerating the translation templates…"
python3 "${SCRIPT_PATH}/updateTemplates.py"


# PO Download #################################################################

echo ":: Downloading translations from Transifex…"
python3 "${SCRIPT_PATH}/pullTranslations.py"


# Pre-Commit Cleanup  #########################################################

echo ":: Reverting unnecessary changes…"
python3 "${SCRIPT_PATH}/checkDiff.py"

# Commit ######################################################################

echo ":: Done"
echo "   Now you can commit your changes to the server."
