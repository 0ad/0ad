import io
import pytest
from checkDiff import check_diff
from unittest import mock
from types import SimpleNamespace

PATCHES = [
"""
Index: binaries/data/l10n/en_GB.engine.po
===================================================================
--- binaries/data/l10n/en_GB.engine.po
+++ binaries/data/l10n/en_GB.engine.po
@@ -103,7 +103,7 @@

 #: lobby/XmppClient.cpp:1291
 msgid "Stream error"
-msgstr "Stream error"
+msgstr "Some Error"

 #: lobby/XmppClient.cpp:1292
 msgid "The incoming stream version is unsupported"

""",
"""
Index: binaries/data/l10n/en_GB.engine.po
===================================================================
--- binaries/data/l10n/en_GB.engine.po
+++ binaries/data/l10n/en_GB.engine.po
@@ -103,7 +103,7 @@

-#: lobby/XmppClient.cpp:1291
+#: lobby/XmppClient.cpp:1295
 msgid "Stream error"
 msgstr "Stream error"
""",
"""
Index: binaries/data/l10n/en_GB.engine.po
===================================================================
--- binaries/data/l10n/en_GB.engine.po
+++ binaries/data/l10n/en_GB.engine.po
@@ -103,7 +103,7 @@

-#: lobby/XmppClient.cpp:1291
+#: lobby/XmppClient.cpp:1295
 msgid "Stream error"
 msgstr "Stream error"
Index: binaries/data/l10n/en_GB_2.engine.po
===================================================================
--- binaries/data/l10n/en_GB_2.engine.po
+++ binaries/data/l10n/en_GB_2.engine.po
@@ -103,7 +103,7 @@

 #: lobby/XmppClient.cpp:1291
 #: lobby/XmppClient.cpp:1295
-msgid "Stream error"
+msgstr "Stretotoro"
Index: binaries/data/l10n/en_GB_3.engine.po
===================================================================
--- binaries/data/l10n/en_GB_3.engine.po
+++ binaries/data/l10n/en_GB_3.engine.po
@@ -103,7 +103,7 @@

-#: lobby/XmppClient.cpp:1291
+#: lobby/XmppClient.cpp:1295
 msgid "Stream error"
 msgstr "Stream error"
""",
"""
Index: binaries/data/l10n/bar.engine.po
===================================================================
--- binaries/data/l10n/bar.engine.po
+++ binaries/data/l10n/bar.engine.po
@@ -3,13 +3,13 @@
 # This file is distributed under the same license as the Pyrogenesis project.
 #
 # Translators:
-# Benedikt Wagner <holledau1@gmx.de>, 2020
+# dabene1408 <holledau1@gmx.de>, 2020
 msgid ""
 msgstr ""
 "Project-Id-Version: 0 A.D.\n"
 "POT-Creation-Date: 2020-05-22 07:08+0000\n"
 "PO-Revision-Date: 2020-06-22 16:38+0000\n"
-"Last-Translator: Benedikt Wagner <holledau1@gmx.de>\n"
+"Last-Translator: dabene1408 <holledau1@gmx.de>\n"
 "Language-Team: Bavarian (http://www.transifex.com/wildfire-games/0ad/language/bar/)\n"
 "MIME-Version: 1.0\n"
 "Content-Type: text/plain; charset=UTF-8\n"
"""
]

PATCHES_EXPECT_REVERT = [
    [],
    ["binaries/data/l10n/en_GB.engine.po"],
    ["binaries/data/l10n/en_GB.engine.po", "binaries/data/l10n/en_GB_3.engine.po"],
    ["binaries/data/l10n/bar.engine.po"]
]

@pytest.fixture(params=zip(PATCHES, PATCHES_EXPECT_REVERT))
def patch(request):
    return [io.StringIO(request.param[0]), set(request.param[1])]


def test_checkdiff(patch):
    assert check_diff(patch[0]) == patch[1]
