#!/bin/bash

# Regenerates the POT files and uploads them to Transifex, downloads the latest
# translations from Transifex, and commits the updated POT and PO files.

SCRIPT_PATH="`dirname \"$0\"`"


# VCS Config ##################################################################

VCS="svn"
VCS_UPDATE="svn update"
VCS_REVERT="svn revert %s@"
VCS_ADD="svn add %s"
VCS_COMMIT_AND_PUSH="svn commit -m '[i18n] Updated POT and PO files.'"
git rev-parse &> /dev/null
if [[ "$?" = "0" ]]; then
    VCS="git"
    VCS_UPDATE="git pull --rebase origin master"
    VCS_REVERT="git checkout -- %s"
    VCS_ADD="git add %s"
    VCS_COMMIT_AND_PUSH="git commit -am '[i18n] Updated POT and PO files.' &&
                         git pull --rebase origin master &&
                         git push origin master"
fi


# Source Update ###############################################################

echo ":: Updating sources…"
${VCS_UPDATE}


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
    if [[ ! -n "$(poediff -c ${VCS} -rHEAD -qs "${FILE_PATH}")" ]]; then
        $(printf "${VCS_REVERT}" "${FILE_PATH}")
    else
        $(printf "${VCS_ADD}" ${FILE_PATH}) &> /dev/null
    fi
done


# Commit ######################################################################

echo ":: Done"
echo "   Now you can commit your changes to the server."
