#!/bin/sh
set -e

die()
{
	echo ERROR: $*
	exit 1
}

# Build the mod .zip using the pyrogenesis executable.
# Assumes it is being run from trunk/

echo "Building archives"

echo "Filtering languages"
# Included languages
# Keep in sync with source/tools/i18n/creditTranslators.py and with the installer languages in 0ad.nsi
LANGS=("ast" "bg" "ca" "cs" "de" "el" "en_GB" "es" "eu" "fr" "gd" "gl" "hu" "id" "it" "ms" "nb" "nl" "pl" "pt_BR" "pt_PT" "ru" "sk" "sv" "tr" "uk")

REGEX=$(printf "\|%s" "${LANGS[@]}")
REGEX=".*/\("${REGEX:2}"\)\.[-A-Za-z0-9_.]\+\.po"

find binaries/ -name "*.po" | grep -v "$REGEX" | xargs rm -v || die "Error filtering languages."

# Build archive(s) - don't archive the _test.* mods
pushd binaries/data/mods > /dev/null
archives=""
ONLY_MOD="${ONLY_MOD:=false}"
if [ "${ONLY_MOD}" = true ]; then
	archives="mod"
else
	for modname in [a-zA-Z0-9]*
	do
		archives="${archives} ${modname}"
	done
fi
popd > /dev/null

for modname in $archives
do
	echo "\nBuilding archive for '${modname}'\n"
	ARCHIVEBUILD_INPUT="binaries/data/mods/${modname}"
	ARCHIVEBUILD_OUTPUT="archives/${modname}"

	mkdir -p "${ARCHIVEBUILD_OUTPUT}"

	(./binaries/system/pyrogenesis -mod=mod -archivebuild="${ARCHIVEBUILD_INPUT}" -archivebuild-output="${ARCHIVEBUILD_OUTPUT}/${modname}.zip") || die "Archive build for '${modname}' failed!"
	cp "${ARCHIVEBUILD_INPUT}/mod.json" "${ARCHIVEBUILD_OUTPUT}" &> /dev/null || true
done
