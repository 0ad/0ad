#!/bin/sh

# Regenerates the POT files, downloads the latest translations from Transifex,
# and prepares the commit of the updated POT and PO files.

SCRIPT_PATH="`dirname \"$0\"`"


# POT Generation ##############################################################

echo ":: Regenerating the translation templates…"
python2 "${SCRIPT_PATH}/updateTemplates.py"


# PO Download #################################################################

echo ":: Downloading translations from Transifex…"
python2 "${SCRIPT_PATH}/pullTranslations.py"


# Pre-Commit Cleanup  #########################################################

# Note: I (Gallaecio) tried using GNU parallel for this, the problem is that
# poediff accesses Subversion, and when you use Subversion more than once
# simultaneously you end up with commands not running properly due to the
# Subversion database being locked. So just take a beverage, put some music on
# and wait for the task to eventually finish.

echo ":: Reverting unnecessary changes…"
for FILE_PATH in $(find "${SCRIPT_PATH}/../../../binaries/data" -name "*.pot" -o -name "*.po")
do
    if [ -z "$(poediff -c svn -qs "${FILE_PATH}")" ]; then
        svn revert "${FILE_PATH}"
    else
        svn add "${FILE_PATH}" 2> /dev/null
    fi
done


# Commit ######################################################################

echo ":: Done"
echo "   Now you can commit your changes to the server."
