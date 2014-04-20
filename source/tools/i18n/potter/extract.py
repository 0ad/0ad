# -*- coding: utf-8 -*-
#
# Copyright (C) 2007-2011 Edgewall Software
# Copyright (C) 2013 Wildfire Games
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without modification, are permitted provided that the
# following conditions are met:
#
#   Redistributions of source code must retain the above copyright notice, this list of conditions and the following
#   disclaimer.
#   Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following
#   disclaimer in the documentation and/or other materials provided with the distribution.
#   The name of the author may not be used to endorse or promote products derived from this software without specific
#   prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR “AS IS” AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
# THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
# AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
# NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
# OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# This software consists of voluntary contributions made by many
# individuals. For the exact contribution history, see the revision
# history and logs:
# • http://babel.edgewall.org/log/trunk/babel/messages
# • http://trac.wildfiregames.com/browser/ps/trunk/source/tools/i18n/potter

"""Basic infrastructure for extracting localizable messages from source files.

This module defines an extensible system for collecting localizable message
strings from a variety of sources. A native extractor for Python source files
is builtin, extractors for other sources can be added using very simple plugins.

The main entry points into the extraction functionality are the functions
`extract_from_dir` and `extract_from_file`.
"""

from __future__ import absolute_import, division, print_function, unicode_literals

__all__ = ['getExtractorInstance']
__docformat__ = 'restructuredtext en'


def getExtractorInstance(code, directoryPath, filemasks, options={}):
    extractorClass = getattr(__import__("potter.extractors", {}, {}, [code,]), code)
    return extractorClass(directoryPath, filemasks, options)
