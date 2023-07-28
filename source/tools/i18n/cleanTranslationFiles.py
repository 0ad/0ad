#!/usr/bin/env python3
#
# Copyright (C) 2023 Wildfire Games.
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

"""
This file removes unneeded personal data from the translators. Most notably
the e-mail addresses. We need to translators' nicks for the credits, but no
more data is required.

TODO: ideally we don't even pull the e-mail addresses in the .po files.
However that needs to be fixed on the transifex side, see rP25896. For now
strip the e-mails using this script.
"""

import sys, os, glob, re, fileinput

from i18n_helper import l10nFolderName, transifexClientFolder, projectRootDirectory

def main():
    # Prepare some regexes.
    translatorMatch = re.compile("(# [^,<]*)(?: <.*>)?(?:, [0-9,-]{4,9})")
    lastTranslatorMatch = re.compile("(\"Last-Translator: [^,<]*)(?: <.*>)?( ?\\\\n\")")

    for root, folders, _ in os.walk(projectRootDirectory):
        for folder in folders:
            if folder == l10nFolderName:
                if os.path.exists(os.path.join(root, folder, transifexClientFolder)):
                    path = os.path.join(root, folder, "*.po")
                    files = glob.glob(path)
                    for file in files:
                        usernames = []
                        reached = False
                        for line in fileinput.input(file.replace("\\", "/"), inplace=True):
                            if reached:
                                if line == "# \n":
                                    line = ""
                                m = translatorMatch.match(line)
                                if m:
                                    if m.group(1) in usernames:
                                        line = ""
                                    else:
                                        line = m.group(1) + "\n"
                                        usernames.append(m.group(1))
                                m2 = lastTranslatorMatch.match(line)
                                if m2:
                                    line = m2.group(1) + "\\n\"\n"
                            elif line.strip() == "# Translators:":
                                reached = True
                            sys.stdout.write(line)

if __name__ == "__main__":
    main()
