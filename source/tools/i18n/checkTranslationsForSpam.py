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

import codecs, os, re, sys

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


def checkTranslationsForSpam(inputFilePath):

    with codecs.open(inputFilePath, 'r', 'utf-8') as fileObject:
        templateCatalog = read_po(fileObject)

    longStringCatalog = Catalog()

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
                    existingTranslationCatalogs.append([filename[:-charactersToSkip], read_po(fileObject)])

    urlPattern = re.compile(u"http[s]?://(?:[a-zA-Z]|[0-9]|[$-_@.&+]|[!*\(\),]|(?:%[0-9a-fA-F][0-9a-fA-F]))+")

    # Check the URLs in translations against the URLs in the translation template.
    for languageCode, translationCatalog in existingTranslationCatalogs:
        for templateMessage in templateCatalog:
            translationMessage = translationCatalog.get(templateMessage.id, templateMessage.context)
            if translationMessage:
                if templateMessage.pluralizable:
                    templateSingularString, templatePluralString = templateMessage.id
                    templateUrls = urlPattern.findall(templateSingularString) # We assume that the same URL is used in both the plural and singular forms.
                    for translationString in translationMessage.string:
                        translationUrls = urlPattern.findall(translationString)
                        for translationUrl in translationUrls:
                            if translationUrl not in templateUrls:
                                print(u"{}: Found the “{}” URL in the translation, which does not match any of the URLs in the translation template: {}".format(
                                        languageCode,
                                        translationUrl,
                                        u", ".join(templateUrls)))
                else:
                    templateUrls = urlPattern.findall(templateMessage.id)
                    translationUrls = urlPattern.findall(translationMessage.string)
                    for translationUrl in translationUrls:
                        if translationUrl not in templateUrls:
                            print(u"{}: Found the “{}” URL in the translation, which does not match any of the URLs in the translation template: {}".format(
                                    languageCode,
                                    translationUrl,
                                    u", ".join(templateUrls)))


def main():

    print(u"\n    WARNING: Remember to regenerate the POT files with “updateTemplates.py” before you run this script.\n    POT files are not in the repository.\n")

    foundPots = 0
    for root, folders, filenames in os.walk(projectRootDirectory):
        root = root.decode("utf-8")
        for filename in filenames:
            if len(filename) > 4 and filename[-4:] == ".pot" and os.path.basename(root) == "l10n":
                foundPots += 1
                checkTranslationsForSpam(os.path.join(root, filename))
    if foundPots == 0:
        print(u"This script did not work because no ‘.pot’ files were found.")
        print(u"Please, run ‘updateTemplates.py’ to generate the ‘.pot’ files, and run ‘pullTranslations.py’ to pull the latest translations from Transifex.")
        print(u"Then you can run this script to generate ‘.po’ files with the longest strings.")


if __name__ == "__main__":
    main()
