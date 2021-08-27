"""Wrapper around babel Catalog / .po handling"""
from datetime import datetime

from babel.messages.catalog import Catalog as BabelCatalog
from babel.messages.pofile import read_po, write_po

class Catalog(BabelCatalog):
    """Wraps a BabelCatalog for convenience."""
    def __init__(self, *args, project=None, copyright_holder=None, **other_kwargs):
        date = datetime.now()
        super().__init__(*args, header_comment=(
                f"# Translation template for {project}.\n"
                f"# Copyright (C) {date.year} {copyright_holder}\n"
                f"# This file is distributed under the same license as the {project} project."
            ),
            copyright_holder=copyright_holder,
            fuzzy=False,
            charset="utf-8",
            creation_date=date,
            revision_date=date,
            **other_kwargs)
        self._project = project

    @BabelCatalog.mime_headers.getter
    def mime_headers(self):
        headers = []
        for name, value in super().mime_headers:
            if name in {
                "PO-Revision-Date",
                "POT-Creation-Date",
                "MIME-Version",
                "Content-Type",
                "Content-Transfer-Encoding",
                "Plural-Forms"}:
                headers.append((name, value))

        return [('Project-Id-Version', self._project)] + headers

    @staticmethod
    def readFrom(file_path, locale = None):
        return read_po(open(file_path, "r+",encoding="utf-8"), locale=locale)

    def writeTo(self, file_path):
        return write_po(
            fileobj=open(file_path, "wb+"),
            catalog=self,
            width=90,
            sort_by_file=True,
        )
