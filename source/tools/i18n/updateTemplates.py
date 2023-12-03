#!/usr/bin/env python3
#
# Copyright (C) 2022 Wildfire Games.
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

import json, os
import multiprocessing
from importlib import import_module

from lxml import etree

from i18n_helper import l10nFolderName, projectRootDirectory
from i18n_helper.catalog import Catalog
from extractors import extractors
messagesFilename = "messages.json"


def warnAboutUntouchedMods():
    """
        Warn about mods that are not properly configured to get their messages extracted.
    """
    modsRootFolder = os.path.join(projectRootDirectory, "binaries", "data", "mods")
    untouchedMods = {}
    for modFolder in os.listdir(modsRootFolder):
        if modFolder[0] != "_" and modFolder[0] != '.':
            if not os.path.exists(os.path.join(modsRootFolder, modFolder, l10nFolderName)):
                untouchedMods[modFolder] = "There is no '{folderName}' folder in the root folder of this mod.".format(folderName=l10nFolderName)
            elif not os.path.exists(os.path.join(modsRootFolder, modFolder, l10nFolderName, messagesFilename)):
                untouchedMods[modFolder] = "There is no '{filename}' file within the '{folderName}' folder in the root folder of this mod.".format(folderName=l10nFolderName, filename=messagesFilename)
    if untouchedMods:
        print(""
            "Warning: No messages were extracted from the following mods:"
            "")
        for mod in untouchedMods:
            print("• {modName}: {warningMessage}".format(modName=mod, warningMessage=untouchedMods[mod]))
        print(""
            f"For this script to extract messages from a mod folder, this mod folder must contain a '{l10nFolderName}' "
            f"folder, and this folder must contain a '{messagesFilename}' file that describes how to extract messages for the "
            f"mod. See the folder of the main mod ('public') for an example, and see the documentation for more "
            f"information."
             )

def generatePOT(templateSettings, rootPath):
    if "skip" in templateSettings and templateSettings["skip"] == "yes":
        return

    inputRootPath = rootPath
    if "inputRoot" in templateSettings:
        inputRootPath = os.path.join(rootPath, templateSettings["inputRoot"])

    template = Catalog(
        project=templateSettings["project"],
        copyright_holder=templateSettings["copyrightHolder"],
        locale='en',
    )

    for rule in templateSettings["rules"]:
        if "skip" in rule and rule["skip"] == "yes":
            return

        options = rule.get("options", {})
        extractorClass = getattr(import_module("extractors.extractors"), rule['extractor'])
        extractor = extractorClass(inputRootPath, rule["filemasks"], options)
        formatFlag = None
        if "format" in options:
            formatFlag = options["format"]
        for message, plural, context, location, comments in extractor.run():
            message_id = (message, plural) if plural else message

            saved_message = template.get(message_id, context) or template.add(
                id=message_id,
                context=context,
                auto_comments=comments,
                flags=[formatFlag] if formatFlag and message.find("%") != -1 else []
            )
            saved_message.locations.append(location)
            saved_message.flags.discard('python-format')

    template.writeTo(os.path.join(rootPath, templateSettings["output"]))
    print(u"Generated \"{}\" with {} messages.".format(templateSettings["output"], len(template)))

def generateTemplatesForMessagesFile(messagesFilePath):

    with open(messagesFilePath, 'r') as fileObject:
        settings = json.load(fileObject)

    for templateSettings in settings:
        multiprocessing.Process(
            target=generatePOT,
            args=(templateSettings, os.path.dirname(messagesFilePath))
        ).start()


def main():
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument("--scandir", help="Directory to start scanning for l10n folders in. "
                                          "Type '.' for current working directory")
    args = parser.parse_args()
    for root, folders, filenames in os.walk(args.scandir or projectRootDirectory):
        for folder in folders:
            if folder == l10nFolderName:
                messagesFilePath = os.path.join(root, folder, messagesFilename)
                if os.path.exists(messagesFilePath):
                    generateTemplatesForMessagesFile(messagesFilePath)

    warnAboutUntouchedMods()


if __name__ == "__main__":
    main()
