#!/usr/bin/env python3
#
# Copyright (C) 2022 Wildfire Games.
# This file is part of 0 A.D.
#
# 0 A.D. is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 2 of the License, or
# (at your option) any later version.
#
# 0 A.D. is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.

import argparse
import os
import sys
import multiprocessing

from i18n_helper import l10nFolderName, projectRootDirectory
from i18n_helper.catalog import Catalog
from i18n_helper.globber import getCatalogs


DEBUG_PREFIX = 'X_X '


def generate_long_strings(root_path, input_file_name, output_file_name, languages=None):
    """
        Generate the 'long strings' debug catalog.
        This catalog contains the longest singular and plural string,
        found amongst all translated languages or a filtered subset.
        It can be used to check if GUI elements are large enough.
        The catalog is long.*.po
    """
    print("Generating", output_file_name)
    input_file_path = os.path.join(root_path, input_file_name)
    output_file_path = os.path.join(root_path, output_file_name)

    template_catalog = Catalog.readFrom(input_file_path)
    # Pretend we write English to get plurals.
    long_string_catalog = Catalog(locale="en")

    # Fill catalog with English strings.
    for message in template_catalog:
        long_string_catalog.add(
            id=message.id, string=message.id, context=message.context)

    # Load existing translation catalogs.
    existing_translation_catalogs = getCatalogs(input_file_path, languages)

    # If any existing translation has more characters than the average expansion, use that instead.
    for translation_catalog in existing_translation_catalogs:
        for long_string_catalog_message in long_string_catalog:
            translation_message = translation_catalog.get(
                long_string_catalog_message.id, long_string_catalog_message.context)
            if not translation_message or not translation_message.string:
                continue

            if not long_string_catalog_message.pluralizable or not translation_message.pluralizable:
                if len(translation_message.string) > len(long_string_catalog_message.string):
                    long_string_catalog_message.string = translation_message.string
                continue

            longest_singular_string = translation_message.string[0]
            longest_plural_string = translation_message.string[1 if len(
                translation_message.string) > 1 else 0]

            candidate_singular_string = long_string_catalog_message.string[0]
            # There might be between 0 and infinite plural forms.
            candidate_plural_string = ""
            for candidate_string in long_string_catalog_message.string[1:]:
                if len(candidate_string) > len(candidate_plural_string):
                    candidate_plural_string = candidate_string

            changed = False
            if len(candidate_singular_string) > len(longest_singular_string):
                longest_singular_string = candidate_singular_string
                changed = True
            if len(candidate_plural_string) > len(longest_plural_string):
                longest_plural_string = candidate_plural_string
                changed = True

            if changed:
                long_string_catalog_message.string = [
                    longest_singular_string, longest_plural_string]
                translation_message = long_string_catalog_message
    long_string_catalog.writeTo(output_file_path)


def generate_debug(root_path, input_file_name, output_file_name):
    """
        Generate a debug catalog to identify untranslated strings.
        This prefixes all strings with DEBUG_PREFIX, to easily identify
        untranslated strings while still making the game navigable.
        The catalog is debug.*.po
    """
    print("Generating", output_file_name)
    input_file_path = os.path.join(root_path, input_file_name)
    output_file_path = os.path.join(root_path, output_file_name)

    template_catalog = Catalog.readFrom(input_file_path)
    # Pretend we write English to get plurals.
    out_catalog = Catalog(locale="en")

    for message in template_catalog:
        if message.pluralizable:
            out_catalog.add(
                id=message.id,
                string=(DEBUG_PREFIX + message.id[0],),
                context=message.context)
        else:
            out_catalog.add(
                id=message.id,
                string=DEBUG_PREFIX + message.id,
                context=message.context)

    out_catalog.writeTo(output_file_path)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--debug",
                        help="Generate debug localisation to identify non-translated strings.",
                        action="store_true")
    parser.add_argument("--long",
                        help="Generate 'long strings' localisation to identify GUI elements too small.",
                        action="store_true")
    parser.add_argument("--languages",
                        nargs="+",
                        help="For long strings, restrict to these languages")
    args = parser.parse_args()

    if not args.debug and not args.long:
        parser.print_help()
        sys.exit(0)

    found_pot_files = 0
    for root, _, filenames in os.walk(projectRootDirectory):
        for filename in filenames:
            if len(filename) > 4 and filename[-4:] == ".pot" and os.path.basename(root) == l10nFolderName:
                found_pot_files += 1
                if args.debug:
                    multiprocessing.Process(
                        target=generate_debug,
                        args=(root, filename, "debug." + filename[:-1])
                    ).start()
                if args.long:
                    multiprocessing.Process(
                        target=generate_long_strings,
                        args=(root, filename, "long." +
                              filename[:-1], args.languages)
                    ).start()

    if found_pot_files == 0:
        print("This script did not work because no ‘.pot’ files were found. "
              "Please, run ‘updateTemplates.py’ to generate the ‘.pot’ files, and run ‘pullTranslations.py’ to pull the latest translations from Transifex. "
              "Then you can run this script to generate ‘.po’ files with obvious debug strings.")


if __name__ == "__main__":
    main()
