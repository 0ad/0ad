#!/bin/bash

# Included languages
# Keep in sync with source/tools/i18n/creditTranslators.py and with the installer languages in 0ad.nsi
LANGS=("ast" "bg" "ca" "cs" "de" "el" "en_GB" "es" "eu" "fr" "gd" "gl" "hu" "id" "it" "ms" "nb" "nl" "pl" "pt_BR" "pt_PT" "ru" "sk" "sv" "tr" "uk")

REGEX=$(printf "\|%s" "${LANGS[@]}")
REGEX=".*/\("${REGEX:2}"\)\.[-A-Za-z0-9_.]\+\.po"

find "$@" -name "*.po" | grep -v "$REGEX" | xargs rm
