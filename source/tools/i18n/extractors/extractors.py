# -*- coding:utf-8 -*-
#
# Copyright (C) 2016 Wildfire Games.
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

from __future__ import absolute_import, division, print_function, unicode_literals

import codecs, re, os, sys
import json as jsonParser

from tokenize import generate_tokens, COMMENT, NAME, OP, STRING
from textwrap import dedent

def pathmatch(mask, path):
    """ Matches paths to a mask, where the mask supports * and **.

    Paths use / as the separator
    * matches a sequence of characters without /.
    ** matches a sequence of characters without / followed by a / and
    sequence of characters without /
    :return: true iff path matches the mask, false otherwise
    """
    s = re.split(r"([*][*]?)", mask)
    p = ""
    for i in xrange(len(s)):
        if i % 2 != 0:
            p = p + "[^/]+"
            if len(s[i]) == 2:
                p = p + "(/[^/]+)*"
        else:
            p = p + re.escape(s[i])
    p = p + "$"
    return re.match(p, path) != None


class Extractor(object):

    def __init__(self, directoryPath, filemasks, options):

        self.directoryPath = directoryPath
        self.options = options

        if isinstance(filemasks, dict):
            self.includeMasks = filemasks["includeMasks"]
            self.excludeMasks = filemasks["excludeMasks"]
        else:
            self.includeMasks = filemasks
            self.excludeMasks = []


    def run(self):
        """ Extracts messages.

        :return:    An iterator over ``(message, plural, context, (location, pos), comment)`` tuples.
        :rtype:     ``iterator``
        """
        directoryAbsolutePath = os.path.abspath(self.directoryPath)
        for root, folders, filenames in os.walk(directoryAbsolutePath):
            for subdir in folders:
                if subdir.startswith('.') or subdir.startswith('_'):
                    folders.remove(subdir)
            folders.sort()
            filenames.sort()
            for filename in filenames:
                filename = os.path.relpath(os.path.join(root, filename), self.directoryPath).replace(os.sep, '/')
                for filemask in self.excludeMasks:
                    if pathmatch(filemask, filename):
                        break
                else:
                    for filemask in self.includeMasks:
                        if pathmatch(filemask, filename):
                            filepath = os.path.join(directoryAbsolutePath, filename)
                            for message, plural, context, breadcrumb, position, comments in self.extractFromFile(filepath):
                                # Replace spaces in filenames by non-breaking spaces so that word
                                # wrapping in po files does not split up our paths
                                yield message, plural, context, (filename.replace(' ', u"\xa0") + (":"+breadcrumb if breadcrumb else ""), position), comments


    def extractFromFile(self, filepath):
        """ Extracts messages from a specific file.

        :return:    An iterator over ``(message, plural, context, breadcrumb, position, comments)`` tuples.
        :rtype:     ``iterator``
        """
        pass



class javascript(Extractor):
    """ Extract messages from JavaScript source code.
    """

    empty_msgid_warning = ( '%s: warning: Empty msgid.  It is reserved by GNU gettext: gettext("") '
                            'returns the header entry with meta information, not the empty string.' )

    def extractJavascriptFromFile(self, fileObject):

        from extractors.jslexer import tokenize, unquote_string
        funcname = message_lineno = None
        messages = []
        last_argument = None
        translator_comments = []
        concatenate_next = False
        last_token = None
        call_stack = -1
        comment_tags = self.options.get('commentTags', [])
        keywords = self.options.get('keywords', {}).keys()

        for token in tokenize(fileObject.read()):
            if token.type == 'operator' and token.value == '(':
                if funcname:
                    message_lineno = token.lineno
                    call_stack += 1

            elif call_stack == -1 and token.type == 'linecomment':
                value = token.value[2:].strip()
                if translator_comments and \
                translator_comments[-1][0] == token.lineno - 1:
                    translator_comments.append((token.lineno, value))
                    continue

                for comment_tag in comment_tags:
                    if value.startswith(comment_tag):
                        translator_comments.append((token.lineno, value.strip()))
                        break

            elif token.type == 'multilinecomment':
                # only one multi-line comment may preceed a translation
                translator_comments = []
                value = token.value[2:-2].strip()
                for comment_tag in comment_tags:
                    if value.startswith(comment_tag):
                        lines = value.splitlines()
                        if lines:
                            lines[0] = lines[0].strip()
                            lines[1:] = dedent('\n'.join(lines[1:])).splitlines()
                            for offset, line in enumerate(lines):
                                translator_comments.append((token.lineno + offset,
                                                            line))
                        break

            elif funcname and call_stack == 0:
                if token.type == 'operator' and token.value == ')':
                    if last_argument is not None:
                        messages.append(last_argument)
                    if len(messages) > 1:
                        messages = tuple(messages)
                    elif messages:
                        messages = messages[0]
                    else:
                        messages = None

                    # Comments don't apply unless they immediately precede the
                    # message
                    if translator_comments and \
                    translator_comments[-1][0] < message_lineno - 1:
                        translator_comments = []

                    if messages is not None:
                        yield (message_lineno, funcname, messages,
                            [comment[1] for comment in translator_comments])

                    funcname = message_lineno = last_argument = None
                    concatenate_next = False
                    translator_comments = []
                    messages = []
                    call_stack = -1

                elif token.type == 'string':
                    new_value = unquote_string(token.value)
                    if concatenate_next:
                        last_argument = (last_argument or '') + new_value
                        concatenate_next = False
                    else:
                        last_argument = new_value

                elif token.type == 'operator':
                    if token.value == ',':
                        if last_argument is not None:
                            messages.append(last_argument)
                            last_argument = None
                        else:
                            messages.append(None)
                        concatenate_next = False
                    elif token.value == '+':
                        concatenate_next = True

            elif call_stack > 0 and token.type == 'operator' \
                and token.value == ')':
                call_stack -= 1

            elif funcname and call_stack == -1:
                funcname = None

            elif call_stack == -1 and token.type == 'name' and \
                token.value in keywords and \
                (last_token is None or last_token.type != 'name' or
                last_token.value != 'function'):
                funcname = token.value

            last_token = token


    def extractFromFile(self, filepath):

        with codecs.open(filepath, 'r', encoding='utf-8-sig') as fileObject:
            for lineno, funcname, messages, comments in self.extractJavascriptFromFile(fileObject):
                if funcname:
                    spec = self.options.get('keywords', {})[funcname] or (1,)
                else:
                    spec = (1,)
                if not isinstance(messages, (list, tuple)):
                    messages = [messages]
                if not messages:
                    continue

                # Validate the messages against the keyword's specification
                context = None
                msgs = []
                invalid = False
                # last_index is 1 based like the keyword spec
                last_index = len(messages)
                for index in spec:
                    if isinstance(index, (list, tuple)):
                        context = messages[index[0] - 1]
                        continue
                    if last_index < index:
                        # Not enough arguments
                        invalid = True
                        break
                    message = messages[index - 1]
                    if message is None:
                        invalid = True
                        break
                    msgs.append(message)
                if invalid:
                    continue

                # keyword spec indexes are 1 based, therefore '-1'
                if isinstance(spec[0], (tuple, list)):
                    # context-aware *gettext method
                    first_msg_index = spec[1] - 1
                else:
                    first_msg_index = spec[0] - 1
                if not messages[first_msg_index]:
                    # An empty string msgid isn't valid, emit a warning
                    where = '%s:%i' % (hasattr(fileObject, 'name') and \
                                        fileObject.name or '(unknown)', lineno)
                    print(self.empty_msgid_warning % where, file=sys.stderr)
                    continue

                messages = tuple(msgs)
                message = messages[0]
                plural = None
                if len(messages) == 2:
                    plural = messages[1]

                yield message, plural, context, None, lineno, comments



class cpp(javascript):
    """ Extract messages from C++ source code.
    """
    pass



class txt(Extractor):
    """ Extract messages from plain text files.
    """

    def extractFromFile(self, filepath):
        with codecs.open(filepath, "r", encoding='utf-8-sig') as fileObject:
            lineCount = 0
            for line in [line.strip("\n\r") for line in fileObject.readlines()]:
                lineCount += 1
                if line:
                    yield line, None, None, None, lineCount, []



class json(Extractor):
    """ Extract messages from JSON files.
    """

    def __init__(self, directoryPath=None, filemasks=[], options={}):
        super(json, self).__init__(directoryPath, filemasks, options)
        self.breadcrumbs = []
        self.keywords = self.options.get("keywords", {})
        self.context = self.options.get("context", None)
        self.comments = self.options.get("comments", [])

    def setOptions(self, options):
        self.options = options
        self.keywords = self.options.get("keywords", {})
        self.context = self.options.get("context", None)
        self.comments = self.options.get("comments", [])

    @staticmethod
    def formatBreadcrumbs(breadcrumbs):
        firstPiece = breadcrumbs[0]
        if isinstance(firstPiece, int): outputString = "[" + str(firstPiece) + "]"
        else: outputString = firstPiece
        for piece in breadcrumbs[1:]:
            if isinstance(piece, int): outputString += "[" + str(piece) + "]"
            else: outputString += "." + piece
        return outputString

    def extractFromFile(self, filepath):
        with codecs.open(filepath, "r", 'utf-8') as fileObject:
            for message, breadcrumbs in self.extractFromString(fileObject.read()):
                yield message, None, self.context, self.formatBreadcrumbs(breadcrumbs), -1, self.comments

    def extractFromString(self, string):
        self.breadcrumbs = []
        jsonDocument = jsonParser.loads(string)
        if isinstance(jsonDocument, list):
            for message, breadcrumbs in self.parseList(jsonDocument):
                if message: # Skip empty strings.
                    yield message, breadcrumbs
        elif isinstance(jsonDocument, dict):
            for message, breadcrumbs in self.parseDictionary(jsonDocument):
                if message: # Skip empty strings.
                    yield message, breadcrumbs
        else:
            raise Exception("Unexpected JSON document parent structure (not a list or a dictionary). You must extend the JSON extractor to support it.")

    def parseList(self, itemsList):
        index = 0
        for listItem in itemsList:
            self.breadcrumbs.append(index)
            if isinstance(listItem, list):
                for message, breadcrumbs in self.parseList(listItem):
                    yield message, breadcrumbs
            elif isinstance(listItem, dict):
                for message, breadcrumbs in self.parseDictionary(listItem):
                    yield message, breadcrumbs
            del self.breadcrumbs[-1]
            index += 1

    def parseDictionary(self, dictionary):
        for keyword in dictionary:
            self.breadcrumbs.append(keyword)
            if keyword in self.keywords:
                if isinstance(dictionary[keyword], unicode):
                    yield dictionary[keyword], self.breadcrumbs
                elif isinstance(dictionary[keyword], list):
                    for message, breadcrumbs in self.extractList(dictionary[keyword]):
                        yield message, breadcrumbs
                elif isinstance(dictionary[keyword], dict):
                    for message, breadcrumbs in self.extractDictionary(dictionary[keyword]):
                        yield message, breadcrumbs
            elif isinstance(dictionary[keyword], list):
                for message, breadcrumbs in self.parseList(dictionary[keyword]):
                    yield message, breadcrumbs
            elif isinstance(dictionary[keyword], dict):
                for message, breadcrumbs in self.parseDictionary(dictionary[keyword]):
                    yield message, breadcrumbs
            del self.breadcrumbs[-1]

    def extractList(self, itemsList):
        index = 0
        for listItem in itemsList:
            self.breadcrumbs.append(index)
            if isinstance(listItem, unicode):
                yield listItem, self.breadcrumbs
            del self.breadcrumbs[-1]
            index += 1

    def extractDictionary(self, dictionary):
        for keyword in dictionary:
            self.breadcrumbs.append(keyword)
            if isinstance(dictionary[keyword], unicode):
                yield dictionary[keyword], self.breadcrumbs
            del self.breadcrumbs[-1]



class xml(Extractor):
    """ Extract messages from XML files.
    """

    def __init__(self, directoryPath, filemasks, options):
        super(xml, self).__init__(directoryPath, filemasks, options)
        self.keywords = self.options.get("keywords", {})
        self.jsonExtractor = None

    def getJsonExtractor(self):
        if not self.jsonExtractor:
            self.jsonExtractor = json()
        return self.jsonExtractor

    def extractFromFile(self, filepath):
        from lxml import etree
        with codecs.open(filepath, "r", encoding='utf-8-sig') as fileObject:
            xmlDocument = etree.parse(fileObject)
            for keyword in self.keywords:
                for element in xmlDocument.iter(keyword):
                    position = element.sourceline
                    if element.text is not None:
                        context = None
                        comments = []
                        if "extractJson" in self.keywords[keyword]:
                            jsonExtractor = self.getJsonExtractor()
                            jsonExtractor.setOptions(self.keywords[keyword]["extractJson"])
                            for message, breadcrumbs in jsonExtractor.extractFromString(element.text):
                                yield message, None, context, json.formatBreadcrumbs(breadcrumbs), position, comments
                        else:
                            breadcrumb = None
                            if "locationAttributes" in self.keywords[keyword]:
                                attributes = [element.get(attribute) for attribute in self.keywords[keyword]["locationAttributes"] if attribute in element.attrib]
                                breadcrumb = "({attributes})".format(attributes=", ".join(attributes))
                            if "tagAsContext" in self.keywords[keyword]:
                                context = keyword
                            if "context" in element.attrib:
                                context = unicode(element.get("context"))
                            if "comment" in element.attrib:
                                comment = element.get("comment")
                                comment = u" ".join(comment.split()) # Remove tabs, line breaks and unecessary spaces.
                                comments.append(comment)
                            if "splitOnWhitespace" in self.keywords[keyword]:
                                for splitText in element.text.split():
                                    # split on whitespace is used for token lists, there, a leading '-' means the token has to be removed, so it's not to be processed here either
                                    if splitText[0] != "-":
                                        yield unicode(splitText), None, context, breadcrumb, position, comments
                            else:
                                yield unicode(element.text), None, context, breadcrumb, position, comments


# Hack from http://stackoverflow.com/a/2819788
class FakeSectionHeader(object):

    def __init__(self, fp):
        self.fp = fp
        self.sechead = '[root]\n'

    def readline(self):
        if self.sechead:
            try: return self.sechead
            finally: self.sechead = None
        else: return self.fp.readline()


class ini(Extractor):
    """ Extract messages from INI files.
    """

    def __init__(self, directoryPath, filemasks, options):
        super(ini, self).__init__(directoryPath, filemasks, options)
        self.keywords = self.options.get("keywords", [])

    def extractFromFile(self, filepath):
        import ConfigParser
        config = ConfigParser.RawConfigParser()
        config.readfp(FakeSectionHeader(open(filepath)))
        for keyword in self.keywords:
            message = config.get("root", keyword).strip('"').strip("'")
            context = None
            position = " ({})".format(keyword)
            comments = []
            yield message, None, context, None, position, comments
