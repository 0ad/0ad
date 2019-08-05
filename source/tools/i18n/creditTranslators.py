#!/usr/bin/env python3
# -*- coding:utf-8 -*-
#
# Copyright (C) 2019 Wildfire Games.
# This file is part of 0 A.D.
#
# 0 A.D. is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 2 of the License, or
# (at your option) any later version.
#
# 0 A.D. is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.

"""
This file updates the translators credits located in the public mod GUI files, using
translators names from the .po files.

If translators change their names on Transifex, the script will remove the old names.
TODO: It should be possible to add people in the list manually, and protect them against
automatic deletion. This has not been needed so far. A possibility would be to add an
optional boolean entry to the dictionary containing the name.

Translatable strings will be extracted from the generated file, so this should be run
once before updateTemplates.py.
"""

import json, os, glob, re

# Credited languages - Keep in sync with source/tools/dist/remove-incomplete-translations.sh
langs = {
    'ast': 'Asturianu',
    'bg': 'Български',
    'ca': 'Català',
    'cs': 'Čeština',
    'de': 'Deutsch',
    'el': 'Ελληνικά',
    'en_GB': 'English (UK)',
    'es': 'Español',
    'eu': 'Euskara',
    'fr': 'Français',
    'gd': 'Gàidhlig',
    'gl': 'Galego',
    'hu': 'Magyar',
    'id': 'Bahasa Indonesia',
    'it': 'Italiano',
    'ms': 'Bahasa Melayu',
    'nb': 'Norsk bokmål',
    'nl': 'Nederlands',
    'pl': 'Polski',
    'pt_BR': 'Português (Brasil)',
    'pt_PT': 'Português (Portugal)',
    'ru': 'Русский',
    'sk': 'Slovenčina',
    'sv': 'Svenska',
    'tr': 'Türkçe',
    'uk': 'Українська'}

root = '../../../'

poLocations = [
    'binaries/data/l10n/',
    'binaries/data/mods/public/l10n/',
    'binaries/data/mods/mod/l10n/']

creditsLocation = 'binaries/data/mods/public/gui/credits/texts/translators.json'

# This dictionnary will hold creditors lists for each language, indexed by code
langsLists = {}

# Create the new JSON data
newJSONData = {'Title': 'Translators', 'Content': []}

# Now go through the list of languages and search the .po files for people

# Prepare some regexes
commentMatch = re.compile('#.*')
translatorMatch = re.compile('# ([^,<]*)(?: <.*>)?, [0-9,-]{4,9}')
deletedUsernameMatch = re.compile('[0-9a-f]{32}')

# Search
for lang in langs.keys():
    if lang not in langsLists.keys():
        langsLists[lang] = []

    for location in poLocations:
        files = glob.glob(root + location + lang + '.*.po')
        for file in files:
            poFile = open(file.replace('\\', '/'))
            reached = False
            for line in poFile:
                if reached:
                    if not commentMatch.match(line):
                        break
                    m = translatorMatch.match(line)
                    if m:
                        username = m.group(1)
                        if not deletedUsernameMatch.match(username):
                            langsLists[lang].append(m.group(1))
                if line.strip() == '# Translators:':
                    reached = True
            poFile.close()

    # Sort and remove duplicates
    # Sorting should ignore case to have a neat credits list
    langsLists[lang] = sorted(set(langsLists[lang]), key=lambda s: s.lower())

# Now insert the new data into the new JSON file
for (langCode, langList) in sorted(langsLists.items()):
    newJSONData['Content'].append({'LangName': langs[langCode], 'List': []})
    for name in langList:
        newJSONData['Content'][-1]['List'].append({'name': name})

# Save the JSON data to the credits file
creditsFile = open(root + creditsLocation, 'w')
json.dump(newJSONData, creditsFile, indent=4)
creditsFile.close()
