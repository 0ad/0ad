#!/bin/bash

# Included languages
# Keep in sync with source/tools/i18n/creditTranslators.py and with the installer languages in 0ad.nsi
LANGS=("bg" "ca" "cs" "de" "en_GB" "es" "fr" "gd" "gl" "hu" "id" "it" "nb" "nl" "pl" "pt_BR" "pt_PT" "ru" "sk" "sv" "tr")

REGEX=$(printf "\|%s" "${LANGS[@]}")
REGEX=".*/\("${REGEX:2}"\)\.[-A-Za-z0-9_.]\+\.po"

find "$@" -name "*.po" | grep -v "$REGEX" | xargs rm
