import os

l10nFolderName = "l10n"
transifexClientFolder = ".tx"
l10nToolsDirectory = os.path.dirname(os.path.realpath(__file__))
projectRootDirectory = os.path.abspath(os.path.join(l10nToolsDirectory, os.pardir, os.pardir, os.pardir, os.pardir))
