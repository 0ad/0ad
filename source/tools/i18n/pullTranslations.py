#!/usr/bin/env python2
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

"""
    Although this script itself should work with both Python 2 and Python 3, it relies on the Transifex Client, which at
    this moment (2013-10-12) does not support Python 3.

    As soon as Transifex Client supports Python 3, simply updating its folder should be enough to make this script work
    with Python 3 as well.
"""

from __future__ import absolute_import, division, print_function, unicode_literals

import os, sys

# Python version check.
if sys.version_info[0] != 2:
    print(__doc__)
    sys.exit()

from txclib.project import Project


def main():


    l10nToolsDirectory = os.path.dirname(os.path.realpath(__file__))
    projectRootDirectory = os.path.abspath(os.path.join(l10nToolsDirectory, os.pardir, os.pardir, os.pardir))
    l10nFolderName = "l10n"
    transifexClientFolder = ".tx"

    for root, folders, filenames in os.walk(projectRootDirectory):
        root = root.decode('utf-8')
        for folder in folders:
            if folder == l10nFolderName:
                if os.path.exists(os.path.join(root, folder, transifexClientFolder)):
                    path = os.path.join(root, folder)
                    os.chdir(path)
                    project = Project(path)
                    project.pull(fetchall=True, force=True)
                    # Use this to pull only the main languages (those that will most likely be included in A16)
                    #project.pull(languages=['en', 'de', 'it', 'pt_PT', 'nl', 'es', 'fr'])



if __name__ == "__main__":
    main()
