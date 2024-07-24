#!/usr/bin/env python3
#
# Copyright (C) 2024 Wildfire Games.
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

import json, os, re
from collections import defaultdict
from pathlib import Path

from babel import Locale, UnknownLocaleError

from i18n_helper import l10nFolderName, transifexClientFolder, projectRootDirectory

poLocations = []
for root, folders, filenames in os.walk(projectRootDirectory):
    for folder in folders:
        if folder == l10nFolderName:
            if os.path.exists(os.path.join(root, folder, transifexClientFolder)):
                poLocations.append(os.path.join(root, folder))

creditsLocation = os.path.join(projectRootDirectory, 'binaries', 'data', 'mods', 'public', 'gui', 'credits', 'texts', 'translators.json')

# This dictionary will hold creditors lists for each language, indexed by code
langsLists = defaultdict(list)

# Create the new JSON data
newJSONData = {'Title': 'Translators', 'Content': []}

# Now go through the list of languages and search the .po files for people

# Prepare some regexes
translatorMatch = re.compile(r"^#\s+([^,<]*)")
deletedUsernameMatch = re.compile(r"[0-9a-f]{32}(_[0-9a-f]{7})?")

# Search
for location in poLocations:
    files = Path(location).glob('*.po')

    for file in files:
        lang = file.stem.split(".")[0]

        # Skip debug translations
        if lang == "debug" or lang == "long":
            continue

        with file.open(encoding='utf-8') as poFile:
            reached = False
            for line in poFile:
                if reached:
                    m = translatorMatch.match(line)
                    if not m:
                        break

                    username = m.group(1)
                    if not deletedUsernameMatch.fullmatch(username):
                        langsLists[lang].append(username)
                if line.strip() == '# Translators:':
                    reached = True

# Sort translator names and remove duplicates
# Sorting should ignore case, but prefer versions of names starting
# with an upper case letter to have a neat credits list.
for lang in langsLists.keys():
    translators = {}
    for name in sorted(langsLists[lang], reverse=True):
        if name.lower() not in translators.keys():
            translators[name.lower()] = name
        elif name.istitle():
            translators[name.lower()] = name
    langsLists[lang] = sorted(translators.values(), key=lambda s: s.lower())

# Now insert the new data into the new JSON file
for langCode, langList in sorted(langsLists.items()):
    try:
        lang_name = Locale.parse(langCode).english_name
    except UnknownLocaleError:
        lang_name = Locale.parse('en').languages.get(langCode)

        if not lang_name:
            raise

    translators = [{'name': name} for name in langList]
    newJSONData['Content'].append({'LangName': lang_name, 'List': translators})

# Sort languages by their English names
newJSONData['Content'] = sorted(newJSONData['Content'], key=lambda x: x['LangName'])

# Save the JSON data to the credits file
creditsFile = open(creditsLocation, 'w', encoding='utf-8')
json.dump(newJSONData, creditsFile, indent=4)
creditsFile.close()
