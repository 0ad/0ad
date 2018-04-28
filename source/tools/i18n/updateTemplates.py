#!/usr/bin/env python2
# -*- coding: utf-8 -*-
#
# Copyright (C) 2018 Wildfire Games.
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

import codecs, datetime, json, os, string, textwrap

from pology.catalog import Catalog
from pology.message import Message
from pology.monitored import Monpair, Monlist

from lxml import etree


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

        template = Catalog(os.path.join(rootPath, templateSettings["output"]), create=True, truncate=True)
        h = template.update_header(
            templateSettings["project"],
            "Translation template for %project.",
            "Copyright (C) {year} {holder}".format(
                year=datetime.datetime.now().year,
                holder=templateSettings["copyrightHolder"]
            ),
            "This file is distributed under the same license as the %project project.",
            plforms="nplurals=2; plural=(n != 1);"
        )
        h.remove_field("Report-Msgid-Bugs-To")
        h.remove_field("Last-Translator")
        h.remove_field("Language-Team")
        h.remove_field("Language")
        h.author = Monlist()

        for rule in templateSettings["rules"]:
            if "skip" in rule and rule["skip"] == "yes":
                continue

            options = rule.get("options", {})
            extractorClass = getattr(__import__("extractors.extractors", {}, {}, [rule["extractor"]]), rule["extractor"])
            extractor = extractorClass(inputRootPath, rule["filemasks"], options)
            formatFlag = None
            if "format" in options:
                formatFlag = options["format"]
            for message, plural, context, location, comments in extractor.run():
                msg = Message({"msgid": message, "msgid_plural": plural, "msgctxt": context, "auto_comment": comments, "flag": [formatFlag] if formatFlag and string.find(message, "%") != -1 else None, "source": [location]})
                if template.get(msg):
                    template.get(msg).source.append(Monpair(location))
                else:
                    template.add(msg)

        template.set_encoding("utf-8")
        template.sync(fitplural=True)
        print(u"Generated \"{}\" with {} messages.".format(templateSettings["output"], len(template)))


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
