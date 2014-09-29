#!/bin/bash

# Included languages
LANGS=("ca" "cs" "de" "en_GB" "es" "fr" "gd" "gl" "it" "nl" "pt_PT" "pt_BR")

REGEX=$(printf "\|%s" "${LANGS[@]}")
REGEX=".*/\("${REGEX:2}"\).[-A-Za-z0-9_.]\+\.po"

find "$@" -name "*.po" | grep -v "$REGEX" | xargs rm
