#!/usr/bin/env python2
# -*- coding:utf-8 -*-
#
# Copyright (C) 2016 Wildfire Games.
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
This file imports the translators credits located in the public mod GUI files and
runs through .po files to add possible new translators to it.
It only appends new people, so it is possible to manually add names in the credits
file and they won't be overwritten by running this script.

Translatable strings will be extracted from the generated file, so this should be ran
before updateTemplates.py.
"""

from __future__ import absolute_import, division, print_function, unicode_literals

import json, os, glob, re

# Credited languages - Keep in sync with source/tools/dist/remove-incomplete-translations.sh
langs = {
    'bg': 'Български',
    'ca': 'Català',
    'cs': 'Ceština',
    'de': 'Deutsch',
    'en_GB': 'English (UK)',
    'es': 'Español',
    'fr': 'Français',
    'gd': 'Gàidhlig',
    'gl': 'Galego',
    'hu': 'Magyar',
    'id': 'Bahasa Indonesia',
    'it': 'Italiano',
    'nl': 'Nederlands',
    'pl': 'Polski',
    'pt_BR': 'Português (Brasil)',
    'pt_PT': 'Português (Portugal)',
    'ru': 'Русский',
    'sk': 'Slovenčina',
    'sv': 'Svenska',
    'tr': 'Türkçe'}

root = '../../../'

poLocations = [
    'binaries/data/l10n/',
    'binaries/data/mods/public/l10n/',
    'binaries/data/mods/mod/l10n/']

creditsLocation = 'binaries/data/mods/public/gui/credits/texts/translators.json'

# Load JSON data
creditsFile = open(root + creditsLocation)
JSONData = json.load(creditsFile)
creditsFile.close()

# This dictionnary will hold creditors lists for each language, indexed by code
langsLists = {}

# Create the new JSON data
newJSONData = {'Content': []}

# First get the already existing lists. If they correspond with some of the credited languages,
# add them to the new data after processing, else add them immediately.
# NB: All of this is quite inefficient
for element in JSONData['Content']:
    if 'LangName' not in element or element['LangName'] not in langs.values():
        newJSONData['Content'].append(element)
        continue

    for (langCode, langName) in langs.items():
        if element['LangName'] == langName:
            langsLists[langCode] = element['List']
            break

# Now actually go through the list of languages and search the .po files for people

# Prepare some regexes
commentMatch = re.compile('#.*')
translatorMatch = re.compile('# ([\w\s]*)(?: <.*>)?, [0-9-]', re.UNICODE)

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
                line = line.decode('utf8')
                if reached:
                    if not commentMatch.match(line):
                        break
                    m = translatorMatch.match(line)
                    if m:
                        langsLists[lang].append(m.group(1))
                if line.strip() == '# Translators:':
                    reached = True
            poFile.close()

    # Sort and remove duplicates
    # Sorting should ignore case to have a neat credits list
    langsLists[lang] = sorted(set(langsLists[lang]), cmp=lambda x,y: cmp(x.lower(), y.lower()))

# Now insert the new data into the new JSON file
for (langCode, langList) in sorted(langsLists.items()):
    newJSONData['Content'].append({'LangName': langs[langCode], 'List': []})
    for name in langList:
        newJSONData['Content'][-1]['List'].append({'name': name})

# Save the JSON data to the credits file
creditsFile = open(root + creditsLocation, 'w')
json.dump(newJSONData, creditsFile, indent=4)
creditsFile.close()
