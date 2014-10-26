#!/usr/bin/env python2
# -*- coding:utf-8 -*-
#
# Copyright (C) 2014 Wildfire Games.
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

from __future__ import absolute_import, division, print_function, unicode_literals

import codecs, json, os, sys, textwrap

from pology.catalog import Catalog
from pology.message import Message


l10nToolsDirectory = os.path.dirname(os.path.realpath(__file__))
projectRootDirectory = os.path.abspath(os.path.join(l10nToolsDirectory, os.pardir, os.pardir, os.pardir))
l10nFolderName = "l10n"


def generateLongStringTranslationFromPotIntoPo(inputFilePath, outputFilePath):

    templateCatalog = Catalog(inputFilePath)
    longStringCatalog = Catalog(outputFilePath, create=True, truncate=True)

    # Fill catalog with English strings.
    for message in templateCatalog:
        longStringCatalog.add(message)

    # If language codes were specified on the command line, filder by those.
    filters = sys.argv[1:]

    # Load existing translation catalogs.
    existingTranslationCatalogs = []
    l10nFolderPath = os.path.dirname(inputFilePath)

    # .pot is one letter longer than .po, but the dot that separates the locale
    # code from the rest of the filename in .po files makes up for that.
    charactersToSkip = len(os.path.basename(inputFilePath))

    for filename in os.listdir(l10nFolderPath):
        if len(filename) > 3 and filename[-3:] == ".po" and filename[:4] != "long":
            if not filters or filename[:-charactersToSkip] in filters:
                if os.path.basename(inputFilePath)[:-4] == filename.split('.')[-2]:
                    existingTranslationCatalogs.append(os.path.join(l10nFolderPath, filename))

    # If any existing translation has more characters than the average expansion, use that instead.
    for pofile in existingTranslationCatalogs:
        print(u"Merging", pofile)
        translationCatalog = Catalog(pofile)
        for longStringCatalogMessage in longStringCatalog:
            translationMessage = translationCatalog.select_by_key(longStringCatalogMessage.msgctxt, longStringCatalogMessage.msgid)
            if not translationMessage:
                continue

            if not longStringCatalogMessage.msgid_plural:
                if len(translationMessage[0].msgstr[0]) > len(longStringCatalogMessage.msgstr[0]):
                    longStringCatalogMessage.msgstr = translationMessage[0].msgstr
                    translationMessage = longStringCatalogMessage
                continue

            longestSingularString = translationMessage[0].msgstr[0]
            longestPluralString = translationMessage[0].msgstr[1] if len(translationMessage[0].msgstr) > 1 else longestSingularString

            candidateSingularString = longStringCatalogMessage.msgstr[0]
            candidatePluralString = "" # There might be between 0 and infinite plural forms.
            for candidateString in longStringCatalogMessage.msgstr[1:]:
                if len(candidateString) > len(candidatePluralString): candidatePluralString = candidateString

            changed = False
            if len(candidateSingularString) > len(longestSingularString):
                longestSingularString = candidateSingularString
                changed = True
            if len(candidatePluralString) > len(longestPluralString):
                longestPluralString   = candidatePluralString
                changed = True

            if changed:
                longStringCatalogMessage.msgstr = [longestSingularString, longestPluralString]
                translationMessage = longStringCatalogMessage

    longStringCatalog.set_encoding("utf-8")
    longStringCatalog.sync()


def main():

    foundPots = 0
    for root, folders, filenames in os.walk(projectRootDirectory):
        root = root.decode("utf-8")
        for filename in filenames:
            if len(filename) > 4 and filename[-4:] == ".pot" and os.path.basename(root) == "l10n":
                foundPots += 1
                print(u"Generating", "long." + filename[:-1])
                generateLongStringTranslationFromPotIntoPo(os.path.join(root, filename), os.path.join(root, "long." + filename[:-1]))
    if foundPots == 0:
        print(u"This script did not work because no ‘.pot’ files were found.")
        print(u"Please, run ‘updateTemplates.py’ to generate the ‘.pot’ files, and run ‘pullTranslations.py’ to pull the latest translations from Transifex.")
        print(u"Then you can run this script to generate ‘.po’ files with the longest strings.")


if __name__ == "__main__":
    main()
