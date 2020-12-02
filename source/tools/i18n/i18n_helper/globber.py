"""Utils to list .po"""
import os
from typing import List

from i18n_helper.catalog import Catalog

def getCatalogs(inputFilePath, filters = None) -> List[Catalog]:
    """Returns a list of "real" catalogs (.po) in the fiven folder."""
    existingTranslationCatalogs = []
    l10nFolderPath = os.path.dirname(inputFilePath)
    inputFileName = os.path.basename(inputFilePath)

    for filename in os.listdir(str(l10nFolderPath)):
        if filename.startswith("long") or not filename.endswith(".po"):
            continue
        if filename.split(".")[1] != inputFileName.split(".")[0]:
            continue
        if not filters or filename.split(".")[0] in filters:
            existingTranslationCatalogs.append(
                Catalog.readFrom(os.path.join(l10nFolderPath, filename), locale=filename.split('.')[0]))

    return existingTranslationCatalogs
