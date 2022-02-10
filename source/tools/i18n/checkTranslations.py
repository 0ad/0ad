#!/usr/bin/env python3
#
# Copyright (C) 2021 Wildfire Games.
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

import sys, os, re, multiprocessing

from i18n_helper import l10nFolderName, projectRootDirectory
from i18n_helper.catalog import Catalog
from i18n_helper.globber import getCatalogs

VERBOSE = 0


class MessageChecker:
    """Checks all messages in a catalog against a regex."""
    def __init__(self, human_name, regex):
        self.regex = re.compile(regex, re.IGNORECASE)
        self.human_name = human_name

    def check(self, inputFilePath, templateMessage, translatedCatalogs):
        patterns = set(self.regex.findall(
            templateMessage.id[0] if templateMessage.pluralizable else templateMessage.id
        ))

        # As a sanity check, verify that the template message is coherent.
        # Note that these tend to be false positives.
        # TODO: the pssible tags are usually comments, we ought be able to find them.
        if templateMessage.pluralizable:
            pluralUrls = set(self.regex.findall(templateMessage.id[1]))
            if pluralUrls.difference(patterns):
                print(f"{inputFilePath} - Different {self.human_name} in singular and plural source strings "
                      f"for '{templateMessage}' in '{inputFilePath}'")

        for translationCatalog in translatedCatalogs:
            translationMessage = translationCatalog.get(
                templateMessage.id, templateMessage.context)
            if not translationMessage:
                continue

            translatedPatterns = set(self.regex.findall(
                translationMessage.string[0] if translationMessage.pluralizable else translationMessage.string
            ))
            unknown_patterns = translatedPatterns.difference(patterns)
            if unknown_patterns:
                print(f'{inputFilePath} - {translationCatalog.locale}: '
                      f'Found unknown {self.human_name} {", ".join(["`" + x + "`" for x in unknown_patterns])} in the translation '
                      f'which do not match any of the URLs in the template: {", ".join(["`" + x + "`" for x in patterns])}')

            if translationMessage.pluralizable:
                for indx, val in enumerate(translationMessage.string):
                    if indx == 0:
                        continue
                    translatedPatternsMulti = set(self.regex.findall(val))
                    unknown_patterns_multi = translatedPatternsMulti.difference(pluralUrls)
                    if unknown_patterns_multi:
                        print(f'{inputFilePath} - {translationCatalog.locale}: '
                              f'Found unknown {self.human_name} {", ".join(["`" + x + "`" for x in unknown_patterns_multi])} in the pluralised translation '
                              f'which do not match any of the URLs in the template: {", ".join(["`" + x + "`" for x in pluralUrls])}')

def check_translations(inputFilePath):
    if VERBOSE:
        print(f"Checking {inputFilePath}")
    templateCatalog = Catalog.readFrom(inputFilePath)

    # If language codes were specified on the command line, filter by those.
    filters = sys.argv[1:]

    # Load existing translation catalogs.
    existingTranslationCatalogs = getCatalogs(inputFilePath, filters)

    spam = MessageChecker("url", r"https?://(?:[a-z0-9-_$@./&+]|(?:%[0-9a-fA-F][0-9a-fA-F]))+")
    sprintf = MessageChecker("sprintf", r"%\([^)]+\)s")
    tags = MessageChecker("tag", r"[^\\][^\\](\[[^]]+/?\])")

    # Check that there are no spam URLs.
    # Loop through all messages in the .POT catalog for URLs.
    # For each, check for the corresponding key in the .PO catalogs.
    # If found, check that URLS in the .PO keys are the same as those in the .POT key.
    for templateMessage in templateCatalog:
        spam.check(inputFilePath, templateMessage, existingTranslationCatalogs)
        sprintf.check(inputFilePath, templateMessage, existingTranslationCatalogs)
        tags.check(inputFilePath, templateMessage, existingTranslationCatalogs)

    if VERBOSE:
        print(f"Done checking {inputFilePath}")


def main():
    print("\n\tWARNING: Remember to regenerate the POT files with “updateTemplates.py” "
          "before you run this script.\n\tPOT files are not in the repository.\n")
    foundPots = 0
    for root, folders, filenames in os.walk(projectRootDirectory):
        for filename in filenames:
            if len(filename) > 4 and filename[-4:] == ".pot" and os.path.basename(root) == l10nFolderName:
                foundPots += 1
                multiprocessing.Process(
                    target=check_translations,
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
