#!/usr/bin/env python3
#
# Copyright (C) 2022 Wildfire Games.
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

from i18n_helper import l10nFolderName, transifexClientFolder, projectRootDirectory

# We credit everyone that helps translating even if the translations don't
# make it into the game.
# Note: Needs to be edited manually when new languages are added on Transifex.
langs = {
    'af': 'Afrikaans',
    'ar': 'الدارجة (Arabic)',
    'ast': 'Asturianu',
    'az': 'Azərbaycan dili',
    'bar': 'Bairisch',
    'be': 'Беларуская мова (Belarusian)',
    'bg': 'Български (Bulgarian)',
    'bn': 'বাংলা (Bengali)',
    'br': 'Brezhoneg',
    'ca': 'Català',
    'cs': 'Čeština ',
    'cy': 'Cymraeg',
    'da': 'Dansk',
    'de': 'Deutsch',
    'el': 'Ελληνικά (Greek)',
    'en_GB': 'English (United Kingdom)',
    'eo': 'Esperanto',
    'es': 'Español',
    'es_AR': 'Español (Argentina)',
    'es_CL': 'Español (Chile)',
    'es_MX': 'Español (Mexico)',
    'et': 'Eesti keel',
    'eu': 'Euskara',
    'fa': 'فارسی (Farsi)',
    'fi': 'Suomi',
    'fr': 'Français',
    'fr_CA': 'Français (Canada)',
    'frp': 'Franco-Provençal (Arpitan)',
    'ga': 'Gaeilge',
    'gd': 'Gàidhlig',
    'gl': 'Galego',
    'he': 'עברית (Hebrew)',
    'hi': 'हिन्दी (Hindi)',
    'hr': 'Croatian',
    'hu': 'Magyar',
    'hy': 'Հայերէն (Armenian)',
    'id': 'Bahasa Indonesia',
    'it': 'Italiano',
    'ja': '日本語 (Japanese)',
    'jbo': 'Lojban',
    'ka': 'ქართული ენა (Georgian)',
    'ko': '한국어 (Korean)',
    'krl': 'Karjalan kieli',
    'ku': 'کوردی (Kurdish)',
    'la': 'Latin',
    'lt': 'Lietuvių kalba',
    'lv': 'Latviešu valoda',
    'mk': 'македонски (Macedonian)',
    'ml': 'മലയാളം (Malayalam)',
    'mr': 'मराठी (Marathi)',
    'ms': 'بهاس ملايو (Malay)',
    'nb': 'Norsk Bokmål',
    'nl': 'Nederlands',
    'pl': 'Polski',
    'pt_BR': 'Português (Brazil)',
    'pt_PT': 'Português (Portugal)',
    'ro': 'Românește',
    'ru': 'Русский язык (Russian)',
    'sk': 'Slovenčina',
    'sl': 'Slovenščina',
    'sq': 'Shqip',
    'sr': 'Cрпски (Serbian)',
    'sv': 'Svenska',
    'szl': 'ślōnskŏ gŏdka',
    'ta_IN': 'தமிழ் (India)',
    'te': 'తెలుగు (Telugu)',
    'th': 'ภาษาไทย (Thai)', 
    'tl': 'Tagalog',
    'tr': 'Türkçe (Turkish)',
    'uk': 'Українська (Ukrainian)',
    'uz': 'Ўзбек тили (Uzbek)',
    'vi': 'Tiếng Việt (Vietnamese)',
    'zh': '中文, 汉语, 漢語 (Chinese)',
    'zh_TW': '臺灣話 Chinese (Taiwan)'}

poLocations = []
for root, folders, filenames in os.walk(projectRootDirectory):
    for folder in folders:
        if folder == l10nFolderName:
            if os.path.exists(os.path.join(root, folder, transifexClientFolder)):
                poLocations.append(os.path.join(root, folder))

creditsLocation = os.path.join(projectRootDirectory, 'binaries', 'data', 'mods', 'public', 'gui', 'credits', 'texts', 'translators.json')

# This dictionnary will hold creditors lists for each language, indexed by code
langsLists = {}

# Create the new JSON data
newJSONData = {'Title': 'Translators', 'Content': []}

# Now go through the list of languages and search the .po files for people

# Prepare some regexes
translatorMatch = re.compile('# (.*)')
deletedUsernameMatch = re.compile('[0-9a-f]{32}')

# Search
for lang in langs.keys():
    if lang not in langsLists.keys():
        langsLists[lang] = []

    for location in poLocations:
        files = glob.glob(os.path.join(location, lang + '.*.po'))
        for file in files:
            poFile = open(file.replace('\\', '/'), encoding='utf-8')
            reached = False
            for line in poFile:
                if reached:
                    m = translatorMatch.match(line)
                    if not m:
                        break

                    username = m.group(1)
                    if not deletedUsernameMatch.match(username):
                        langsLists[lang].append(username)
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
creditsFile = open(creditsLocation, 'w', encoding='utf-8')
json.dump(newJSONData, creditsFile, indent=4)
creditsFile.close()
