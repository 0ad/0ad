#!/bin/bash
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
# CJK languages are excluded, as they are in mods.
# Note: Needs to be edited manually at each release.
# Keep in sync with the installer languages in 0ad.nsi.
LANGS=("ast" "ca" "cs" "de" "el" "en_GB" "es" "eu" "fi" "fr" "gd" "hu" "id" "it" "nl" "pl" "pt_BR" "ru" "sk" "sv" "tr" "uk")

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

BUILD_SHADERS="${BUILD_SHADERS:=true}"
if [ "${BUILD_SHADERS}" = true ]; then
	PYTHON=${PYTHON:=$(command -v python3 || command -v python)}
	GLSLC=${GLSLC:=$(command -v glslc)}
	SPIRV_REFLECT=${SPIRV_REFLECT:=$(command -v spirv-reflect)}

	[ -n "${PYTHON}" ] || die "Error: python is not available. Install it before proceeding."
	[ -n "${GLSLC}" ] || die "Error: glslc is not available. Install it with the Vulkan SDK before proceeding."
	[ -n "${SPIRV_REFLECT}" ] || die "Error: spirv-reflect is not available. Install it with the Vulkan SDK before proceeding."

	pushd "source/tools/spirv" > /dev/null

	ENGINE_VERSION=${ENGINE_VERSION:="0.0.xx"}
	rulesFile="rules.${ENGINE_VERSION}.json"

	if [ ! -e "$rulesFile" ]
	then
		# The rules.json file should be present in release tarballs, for
		# some Linux CIs don't have access to the internet.
		download="$(command -v wget || echo "curl -sLo ""${rulesFile}""")"
		$download "https://releases.wildfiregames.com/spir-v/$rulesFile"
	fi

	for modname in $archives
	do
		modLocation="../../../binaries/data/mods/${modname}"
		if [ -e "${modLocation}/shaders/spirv/" ]
		then
			echo "Removing existing spirv shaders for '${modname}'..."
			rm -rf "${modLocation}/shaders/spirv"
		fi
		echo "Building shader for '${modname}'..."
		$PYTHON compile.py "$modLocation" "$rulesFile" "$modLocation" --dependency "../../../binaries/data/mods/mod/"
	done
	popd > /dev/null
fi

for modname in $archives
do
	echo "Building archive for '${modname}'..."
	ARCHIVEBUILD_INPUT="binaries/data/mods/${modname}"
	ARCHIVEBUILD_OUTPUT="archives/${modname}"

	mkdir -p "${ARCHIVEBUILD_OUTPUT}"

	(./binaries/system/pyrogenesis -mod=mod -archivebuild="${ARCHIVEBUILD_INPUT}" -archivebuild-output="${ARCHIVEBUILD_OUTPUT}/${modname}.zip") || die "Archive build for '${modname}' failed!"
	cp "${ARCHIVEBUILD_INPUT}/mod.json" "${ARCHIVEBUILD_OUTPUT}" &> /dev/null || true
done
