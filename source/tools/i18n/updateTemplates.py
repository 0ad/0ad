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

import codecs, json, os, textwrap

from potter.catalog import Catalog, Message
from potter.extract import getExtractorInstance
from potter.pofile import write_po


l10nToolsDirectory = os.path.dirname(os.path.realpath(__file__))
projectRootDirectory = os.path.abspath(os.path.join(l10nToolsDirectory, os.pardir, os.pardir, os.pardir))
l10nFolderName = "l10n"
messagesFilename = "messages.json"


def warnAboutUntouchedMods():
    """
        Warn about mods that are not properly configured to get their messages extracted.
    """
    modsRootFolder = os.path.join(projectRootDirectory, "binaries", "data", "mods")
    untouchedMods = {}
    for modFolder in os.listdir(modsRootFolder):
        if modFolder[0] != "_":
            if not os.path.exists(os.path.join(modsRootFolder, modFolder, l10nFolderName)):
                untouchedMods[modFolder] = "There is no '{folderName}' folder in the root folder of this mod.".format(folderName=l10nFolderName)
            elif not os.path.exists(os.path.join(modsRootFolder, modFolder, l10nFolderName, messagesFilename)):
                untouchedMods[modFolder] = "There is no '{filename}' file within the '{folderName}' folder in the root folder of this mod.".format(folderName=l10nFolderName, filename=messagesFilename)
    if untouchedMods:
        print(textwrap.dedent("""
                Warning: No messages were extracted from the following mods:
            """))
        for mod in untouchedMods:
            print("• {modName}: {warningMessage}".format(modName=mod, warningMessage=untouchedMods[mod]))
        print(textwrap.dedent("""
                For this script to extract messages from a mod folder, this mod folder must contain a '{folderName}'
                folder, and this folder must contain a '{filename}' file that describes how to extract messages for the
                mod. See the folder of the main mod ('public') for an example, and see the documentation for more
                information.
                """.format(folderName=l10nFolderName, filename=messagesFilename)
             ))


def generateTemplatesForMessagesFile(messagesFilePath):

    with open(messagesFilePath, 'r') as fileObject:
        settings = json.load(fileObject)

    rootPath = os.path.dirname(messagesFilePath)

    for templateSettings in settings:

        if "skip" in templateSettings and templateSettings["skip"] == "yes":
            continue

        inputRootPath = rootPath
        if "inputRoot" in templateSettings:
            inputRootPath = os.path.join(rootPath, templateSettings["inputRoot"])

        template = Catalog()
        template.project = templateSettings["project"]
        template.copyright_holder = templateSettings["copyrightHolder"]

        for rule in templateSettings["rules"]:

            if "skip" in rule and rule["skip"] == "yes":
                continue

            options = rule.get("options", {})
            extractor = getExtractorInstance(rule["extractor"], inputRootPath, rule["filemasks"], options)
            for message, context, location, comments in extractor.run():
                formatFlag = None
                if "format" in options:
                    formatFlag = options["format"]
                template.add(message, context=context, locations=[location], auto_comments=comments, formatFlag=formatFlag)

        with codecs.open(os.path.join(rootPath, templateSettings["output"]), 'w', 'utf-8') as fileObject:
            write_po(fileObject, template)

        print(u"Generated “{}” with {} messages.".format(templateSettings["output"], len(template)))


def main():

    for root, folders, filenames in os.walk(projectRootDirectory):
        for folder in folders:
            if folder == l10nFolderName:
                messagesFilePath = os.path.join(root, folder, messagesFilename)
                if os.path.exists(messagesFilePath):
                    generateTemplatesForMessagesFile(messagesFilePath)

    warnAboutUntouchedMods()


if __name__ == "__main__":
    main()
