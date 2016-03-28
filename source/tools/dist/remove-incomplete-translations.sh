#!/bin/bash

# Included languages
LANGS=("bg" "ca" "cs" "de" "en_GB" "es" "fr" "gd" "gl" "hu" "id" "it" "nl" "pl" "pt_BR" "pt_PT" "ru" "sk" "sv" "tr")

REGEX=$(printf "\|%s" "${LANGS[@]}")
REGEX=".*/\("${REGEX:2}"\)\.[-A-Za-z0-9_.]\+\.po"

find "$@" -name "*.po" | grep -v "$REGEX" | xargs rm
