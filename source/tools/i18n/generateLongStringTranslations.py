#!/usr/bin/env python
# -*- coding:utf-8 -*-
#
# Copyright (C) 2013 Wildfire Games.
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

from potter.catalog import Catalog, Message
from potter.extract import getExtractorInstance
from potter.pofile import read_po, write_po


l10nToolsDirectory = os.path.dirname(os.path.realpath(__file__))
projectRootDirectory = os.path.abspath(os.path.join(l10nToolsDirectory, os.pardir, os.pardir, os.pardir))
l10nFolderName = "l10n"


#def getAverageExpansionForEnglishString(string):
    #"""
        #Based on http://www.w3.org/International/articles/article-text-size.en
    #"""
    #length = len(string)
    #if len <= 10:
        #return length*3     # 200–300%
    #if len <= 20:
        #return length*2     # 180–200%
    #if len <= 30:
        #return length*1.8   # 160–180%
    #if len <= 50:
        #return length*1.6   # 140–160%
    #if len <= 70:
        #return length*1.7   # 151-170%

    #return length*1.3       # 130%


#def enlarge(string, surroundWithSpaces):
    #halfExpansion = int(getAverageExpansionForEnglishString(string)/2)
    #if surroundWithSpaces: halfExpansion -= 1

    #outputString = "x"*halfExpansion
    #if surroundWithSpaces:
        #outputString += " "

    #outputString += string

    #if surroundWithSpaces:
        #outputString += " "
    #outputString += "x"*halfExpansion

    #return outputString


def generateLongStringTranslationFromPotIntoPo(inputFilePath, outputFilePath):

    with codecs.open(inputFilePath, 'r', 'utf-8') as fileObject:
        templateCatalog = read_po(fileObject)

    longStringCatalog = Catalog()

    # Fill catalog with English strings.
    for message in templateCatalog:
        if message.pluralizable:
            singularString, pluralString = message.id
            message.string = (singularString, pluralString)
        else:
            message.string = message.id
        longStringCatalog[message.id] = message

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
                with codecs.open(os.path.join(l10nFolderPath, filename), 'r', 'utf-8') as fileObject:
                    existingTranslationCatalogs.append(read_po(fileObject))

    # If any existing translation has more characters than the average expansion, use that instead.
    for translationCatalog in existingTranslationCatalogs:
        for longStringCatalogMessage in longStringCatalog:
            translationMessage = translationCatalog.get(longStringCatalogMessage.id, longStringCatalogMessage.context)
            if translationMessage:
                if longStringCatalogMessage.pluralizable:
                    currentSingularString, currentPluralString = longStringCatalogMessage.string
                    longestSingularString = currentSingularString
                    longestPluralString = currentPluralString

                    candidateSingularString = translationMessage.string[0]
                    candidatePluralString = "" # There might be between 0 and infinite plural forms.
                    for candidateString in translationMessage.string[1:]:
                        if len(candidateString) > len(candidatePluralString): candidatePluralString = candidateString

                    changed = False
                    if len(candidateSingularString) > len(currentSingularString):
                        longestSingularString = candidateSingularString
                        changed = True
                    if len(candidatePluralString) > len(currentPluralString):
                        longestPluralString   = candidatePluralString
                        changed = True

                    if changed:
                        longStringCatalogMessage.string = (longestSingularString, longestPluralString)
                        longStringCatalog[longStringCatalogMessage.id] = longStringCatalogMessage

                else:
                    if len(translationMessage.string) > len(longStringCatalogMessage.string):
                        longStringCatalogMessage.string = translationMessage.string
                        longStringCatalog[longStringCatalogMessage.id] = longStringCatalogMessage


    with codecs.open(outputFilePath, 'w', 'utf-8') as fileObject:
        write_po(fileObject, longStringCatalog)


def main():

    foundPots = 0
    for root, folders, filenames in os.walk(projectRootDirectory):
        root = root.decode("utf-8")
        for filename in filenames:
            if len(filename) > 4 and filename[-4:] == ".pot" and os.path.basename(root) == "l10n":
                foundPots += 1
                generateLongStringTranslationFromPotIntoPo(os.path.join(root, filename), os.path.join(root, "long." + filename[:-1]))
    if foundPots == 0:
        print(u"This script did not work because no ‘.pot’ files were found.")
        print(u"Please, run ‘updateTemplates.py’ to generate the ‘.pot’ files, and run ‘pullTranslations.py’ to pull the latest translations from Transifex.")
        print(u"Then you can run this script to generate ‘.po’ files with the longest strings.")


if __name__ == "__main__":
    main()
