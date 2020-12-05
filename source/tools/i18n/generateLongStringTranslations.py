#!/usr/bin/env python3
#
# Copyright (C) 2020 Wildfire Games.
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

import os, sys
import multiprocessing

from i18n_helper import l10nToolsDirectory, projectRootDirectory
from i18n_helper.catalog import Catalog
from i18n_helper.globber import getCatalogs

l10nFolderName = "l10n"


def generateLongStringTranslationFromPotIntoPo(inputFilePath, outputFilePath):
    templateCatalog = Catalog.readFrom(inputFilePath)
    longStringCatalog = Catalog(locale="en") # Pretend we write English to get plurals.

    # Fill catalog with English strings.
    for message in templateCatalog:
        longStringCatalog.add(id=message.id, string=message.id, context=message.context)

    # If language codes were specified on the command line, filder by those.
    filters = sys.argv[1:]

    # Load existing translation catalogs.
    existingTranslationCatalogs = getCatalogs(inputFilePath, filters)

    # If any existing translation has more characters than the average expansion, use that instead.
    for translationCatalog in existingTranslationCatalogs:
        for longStringCatalogMessage in longStringCatalog:
            translationMessage = translationCatalog.get(longStringCatalogMessage.id, longStringCatalogMessage.context)
            if not translationMessage or not translationMessage.string:
                continue

            if not longStringCatalogMessage.pluralizable or not translationMessage.pluralizable:
                if len(translationMessage.string) > len(longStringCatalogMessage.string):
                    longStringCatalogMessage.string = translationMessage.string
                continue

            longestSingularString = translationMessage.string[0]
            longestPluralString = translationMessage.string[1 if len(translationMessage.string) > 1 else 0]

            candidateSingularString = longStringCatalogMessage.string[0]
            candidatePluralString = "" # There might be between 0 and infinite plural forms.
            for candidateString in longStringCatalogMessage.string[1:]:
                if len(candidateString) > len(candidatePluralString):
                    candidatePluralString = candidateString

            changed = False
            if len(candidateSingularString) > len(longestSingularString):
                longestSingularString = candidateSingularString
                changed = True
            if len(candidatePluralString) > len(longestPluralString):
                longestPluralString   = candidatePluralString
                changed = True

            if changed:
                longStringCatalogMessage.string = [longestSingularString, longestPluralString]
                translationMessage = longStringCatalogMessage
    longStringCatalog.writeTo(outputFilePath)


def main():

    foundPots = 0
    for root, folders, filenames in os.walk(projectRootDirectory):
        for filename in filenames:
            if len(filename) > 4 and filename[-4:] == ".pot" and os.path.basename(root) == "l10n":
                foundPots += 1
                print("Generating", "long." + filename[:-1])
                multiprocessing.Process(
                    target=generateLongStringTranslationFromPotIntoPo,
                    args=(os.path.join(root, filename), os.path.join(root, "long." + filename[:-1]))
                ).start()

    if foundPots == 0:
        print("This script did not work because no ‘.pot’ files were found. "
              "Please, run ‘updateTemplates.py’ to generate the ‘.pot’ files, and run ‘pullTranslations.py’ to pull the latest translations from Transifex. "
              "Then you can run this script to generate ‘.po’ files with the longest strings.")


if __name__ == "__main__":
    main()
