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

import os, re, sys
import multiprocessing

from i18n_helper import l10nToolsDirectory, projectRootDirectory
from i18n_helper.catalog import Catalog
from i18n_helper.globber import getCatalogs

l10nFolderName = "l10n"

def checkTranslationsForSpam(inputFilePath):
    print(f"Checking {inputFilePath}")
    templateCatalog = Catalog.readFrom()

    # If language codes were specified on the command line, filter by those.
    filters = sys.argv[1:]

    # Load existing translation catalogs.
    existingTranslationCatalogs = getCatalogs(inputFilePath, filters)

    urlPattern = re.compile(r"https?://(?:[a-z0-9-_$@./&+]|(?:%[0-9a-fA-F][0-9a-fA-F]))+", re.IGNORECASE)

    # Check that there are no spam URLs.
    # Loop through all messages in the .POT catalog for URLs.
    # For each, check for the corresponding key in the .PO catalogs.
    # If found, check that URLS in the .PO keys are the same as those in the .POT key.
    for templateMessage in templateCatalog:
        templateUrls = set(urlPattern.findall(
            templateMessage.id[0] if templateMessage.pluralizable else templateMessage.id
        ))
        # As a sanity check, verify that the template message is coherent
        if templateMessage.pluralizable:
            pluralUrls = set(urlPattern.findall(templateMessage.id[1]))
            if pluralUrls.difference(templateUrls):
                print(f"{inputFilePath} - Different URLs in singular and plural source strings "
                      f"for '{templateMessage}' in '{inputFilePath}'")

        for translationCatalog in existingTranslationCatalogs:
            translationMessage = translationCatalog.get(templateMessage.id, templateMessage.context)
            if not translationMessage:
                continue

            translationUrls = set(urlPattern.findall(
                translationMessage.string[0] if translationMessage.pluralizable else translationMessage.string
            ))
            unknown_urls = translationUrls.difference(templateUrls)
            if unknown_urls:
                print(f'{inputFilePath} - {translationCatalog.locale}: '
                      f'Found unknown URL(s) {", ".join(unknown_urls)} in the translation '
                      f'which do not match any of the URLs in the template: {", ".join(templateUrls)}')
    print(f"Done checking {inputFilePath}")

def main():
    print("\n\tWARNING: Remember to regenerate the POT files with “updateTemplates.py” "
          "before you run this script.\n\tPOT files are not in the repository.\n")
    foundPots = 0
    for root, folders, filenames in os.walk(projectRootDirectory):
        for filename in filenames:
            if len(filename) > 4 and filename[-4:] == ".pot" and os.path.basename(root) == "l10n":
                foundPots += 1
                multiprocessing.Process(
                    target=checkTranslationsForSpam,
                    args=(os.path.join(root, filename), )
                ).start()
    if foundPots == 0:
        print(
            "This script did not work because no '.pot' files were found. "
            "Please run 'updateTemplates.py' to generate the '.pot' files, "
            "and run 'pullTranslations.py' to pull the latest translations from Transifex. "
            "Then you can run this script to check for spam in translations.")


if __name__ == "__main__":
    main()
