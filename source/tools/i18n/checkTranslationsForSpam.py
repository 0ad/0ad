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

import codecs, os, re, sys

from pology.catalog import Catalog
from pology.message import Message


l10nToolsDirectory = os.path.dirname(os.path.realpath(__file__))
projectRootDirectory = os.path.abspath(os.path.join(l10nToolsDirectory, os.pardir, os.pardir, os.pardir))
l10nFolderName = "l10n"


def checkTranslationsForSpam(inputFilePath):

    print(u"Checking", inputFilePath)
    templateCatalog = Catalog(inputFilePath)

    # If language codes were specified on the command line, filter by those.
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
                    existingTranslationCatalogs.append([filename[:-charactersToSkip], os.path.join(l10nFolderPath, filename)])

    urlPattern = re.compile(u"http[s]?://(?:[a-zA-Z]|[0-9]|[$-_@.&+]|[!*\(\),]|(?:%[0-9a-fA-F][0-9a-fA-F]))+")

    # Check the URLs in translations against the URLs in the translation template.
    for languageCode, pofile in existingTranslationCatalogs:
        translationCatalog = Catalog(pofile)
        for templateMessage in templateCatalog:
            translationMessage = translationCatalog.select_by_key(templateMessage.msgctxt, templateMessage.msgid)
            if translationMessage:
                templateSingularString = templateMessage.msgid
                templateUrls = urlPattern.findall(templateMessage.msgid)
                # Assert that the same URL is used in both the plural and singular forms.
                if templateMessage.msgid_plural and len(templateMessage.msgstr) > 1:
                    pluralUrls = urlPattern.findall(templateMessage.msgstr[0])
                    for url in pluralUrls:
                        if url not in templateUrls:
                            print(u"Different URLs in singular and plural source strings for ‘{}’ in ‘{}’".format(
                                templateMessage.msgid,
                                inputFilePath))
                for translationString in translationMessage[0].msgstr:
                    translationUrls = urlPattern.findall(translationString)
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
