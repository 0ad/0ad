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

"""Data structures for message catalogs."""

from __future__ import absolute_import, division, print_function, unicode_literals

from cgi import parse_header
from datetime import datetime, time as time_
from difflib import get_close_matches
from email import message_from_string
from copy import copy
import re
import time

from collections import OrderedDict

from potter.util import distinct, LOCALTZ, UTC, FixedOffsetTimezone


__all__ = ['Message', 'Catalog']
__docformat__ = 'restructuredtext en'


PYTHON_FORMAT = re.compile(r"""(?x)
    \%
        (?:\(([\w]*)\))?
        (
            [-#0\ +]?(?:\*|[\d]+)?
            (?:\.(?:\*|[\d]+))?
            [hlL]?
        )
        ([diouxXeEfFgGcrs%])
""")

C_FORMAT = re.compile(r"""(?x)
    \%
        (\d+\$)?
        ([-+ 0#]+)?
        (v|\*(\d+\$)?v)?
        0*
        (\d+|\*(\d+\$)?)?
        (\.(\d*|\*(\d+\$)?))?
        [hlqLV]?
        ([%bcdefginopsuxDFOUX])
""")


class Message(object):
    """Representation of a single message in a catalog."""

    def __init__(self, id, string=u'', locations=(), flags=(), auto_comments=(),
                 user_comments=(), previous_id=(), lineno=None, context=None, formatFlag=None):
        """Create the message object.

        :param id: the message ID, or a ``(singular, plural)`` tuple for
                   pluralizable messages
        :param string: the translated message string, or a
                       ``(singular, plural)`` tuple for pluralizable messages
        :param locations: a sequence of ``(filenname, lineno)`` tuples
        :param flags: a set or sequence of flags
        :param auto_comments: a sequence of automatic comments for the message
        :param user_comments: a sequence of user comments for the message
        :param previous_id: the previous message ID, or a ``(singular, plural)``
                            tuple for pluralizable messages
        :param lineno: the line number on which the msgid line was found in the
                       PO file, if any
        :param context: the message context
        """
        self.id = id #: The message ID
        if not string and self.pluralizable:
            string = (u'', u'')
        self.string = string #: The message translation
        self.locations = list(distinct(locations))
        self.flags = set(flags)
        if id and formatFlag is None:
            formatFlag = self.guessFormatFlag();
            if formatFlag:
                self.flags.add(formatFlag)
        self.auto_comments = list(distinct(auto_comments))
        self.user_comments = list(distinct(user_comments))
        if isinstance(previous_id, str):
            self.previous_id = [previous_id]
        else:
            self.previous_id = list(previous_id)
        self.lineno = lineno
        self.context = context

    def __repr__(self):
        return '<%s %r (flags: %r)>' % (type(self).__name__, self.id,
                                        list(self.flags))

    def __cmp__(self, obj):
        """Compare Messages, taking into account plural ids"""
        def values_to_compare():
            if isinstance(obj, Message):
                plural = self.pluralizable
                obj_plural = obj.pluralizable
                if plural and obj_plural:
                    return self.id[0], obj.id[0]
                elif plural:
                    return self.id[0], obj.id
                elif obj_plural:
                    return self.id, obj.id[0]
            return self.id, obj.id
        this, other = values_to_compare()
        return cmp(this, other)

    def __gt__(self, other):
        return self.__cmp__(other) > 0

    def __lt__(self, other):
        return self.__cmp__(other) < 0

    def __ge__(self, other):
        return self.__cmp__(other) >= 0

    def __le__(self, other):
        return self.__cmp__(other) <= 0

    def __eq__(self, other):
        return self.__cmp__(other) == 0

    def __ne__(self, other):
        return self.__cmp__(other) != 0

    def clone(self):
        return Message(*map(copy, (self.id, self.string, self.locations,
                                   self.flags, self.auto_comments,
                                   self.user_comments, self.previous_id,
                                   self.lineno, self.context)))

    @property
    def pluralizable(self):
        """Whether the message is plurizable.

        >>> Message('foo').pluralizable
        False
        >>> Message(('foo', 'bar')).pluralizable
        True

        :type:  `bool`"""
        return isinstance(self.id, (list, tuple))

    def guessFormatFlag(self):
        """ If the message contains parameters, this function returns a string with the flag that represents the format
            of those parameters.

        :type:  `string`"""
        ids = self.id
        if not isinstance(ids, (list, tuple)):
            ids = [ids]
        for id in ids:
            if C_FORMAT.search(id) is not None:
                return "c-format"
        for id in ids:
            if PYTHON_FORMAT.search(id) is not None:
                return "python-format"
        return None


DEFAULT_HEADER = u"""\
# Translation template for PROJECT.
# Copyright © YEAR ORGANIZATION
# This file is distributed under the same license as the PROJECT project.
#"""


class Catalog(object):
    """Representation of a message catalog."""

    def __init__(self, locale=None, domain=None, header_comment=DEFAULT_HEADER,
                 project=None, version=None, copyright_holder=None,
                 msgid_bugs_address=None, creation_date=None,
                 revision_date=None, charset='utf-8'):
        """Initialize the catalog object.

        :param domain: the message domain
        :param header_comment: the header comment as string, or `None` for the
                               default header
        :param project: the project's name
        :param version: the project's version
        :param copyright_holder: the copyright holder of the catalog
        :param msgid_bugs_address: the email address or URL to submit bug
                                   reports to
        :param creation_date: the date the catalog was created
        :param revision_date: the date the catalog was revised
        :param charset: the encoding to use in the output
        """
        self.domain = domain #: The message domain
        self._header_comment = header_comment
        self._messages = OrderedDict()

        self.project = project or 'PROJECT' #: The project name
        self.version = version #: The project version
        self.copyright_holder = copyright_holder or 'ORGANIZATION'
        self.msgid_bugs_address = msgid_bugs_address or 'EMAIL@ADDRESS'

        self.charset = charset or 'utf-8'

        if creation_date is None:
            creation_date = datetime.now(LOCALTZ)
        elif isinstance(creation_date, datetime) and not creation_date.tzinfo:
            creation_date = creation_date.replace(tzinfo=LOCALTZ)
        self.creation_date = creation_date #: Creation date of the template
        if revision_date is None:
            revision_date = 'YEAR-MO-DA HO:MI+ZONE'
        elif isinstance(revision_date, datetime) and not revision_date.tzinfo:
            revision_date = revision_date.replace(tzinfo=LOCALTZ)
        self.revision_date = revision_date #: Last revision date of the catalog

        self.obsolete = OrderedDict() #: Dictionary of obsolete messages
        self._num_plurals = None
        self._plural_expr = None

    def _get_header_comment(self):
        comment = self._header_comment
        year = datetime.now(LOCALTZ).strftime('%Y')
        if hasattr(self.revision_date, 'strftime'):
            year = self.revision_date.strftime('%Y')
        comment = comment.replace('PROJECT', self.project) \
                         .replace('YEAR', year) \
                         .replace('ORGANIZATION', self.copyright_holder)
        return comment

    def _set_header_comment(self, string):
        self._header_comment = string

    header_comment = property(_get_header_comment, _set_header_comment, doc="""\
    The header comment for the catalog.

    >>> catalog = Catalog(project='Foobar', version='1.0',
    ...                   copyright_holder='Foo Company')
    >>> print catalog.header_comment #doctest: +ELLIPSIS
    # Translations template for Foobar.
    # Copyright (C) ... Foo Company
    # This file is distributed under the same license as the Foobar project.
    # FIRST AUTHOR <EMAIL@ADDRESS>, ....
    #

    The header can also be set from a string. Any known upper-case variables
    will be replaced when the header is retrieved again:

    >>> catalog = Catalog(project='Foobar', version='1.0',
    ...                   copyright_holder='Foo Company')
    >>> catalog.header_comment = '''\\
    ... # The POT for my really cool PROJECT project.
    ... # Copyright (C) 1990-2003 ORGANIZATION
    ... # This file is distributed under the same license as the PROJECT
    ... # project.
    ... #'''
    >>> print catalog.header_comment
    # The POT for my really cool Foobar project.
    # Copyright (C) 1990-2003 Foo Company
    # This file is distributed under the same license as the Foobar
    # project.
    #

    :type: `unicode`
    """)

    def _get_mime_headers(self):
        headers = []
        projectIdVersion = self.project
        if self.version:
            projectIdVersion += " " + self.version
        headers.append(('Project-Id-Version', projectIdVersion))
        headers.append(('Report-Msgid-Bugs-To', self.msgid_bugs_address))
        headers.append(('POT-Creation-Date', self.creation_date.strftime('%Y-%m-%d %H:%M%z')))
        if isinstance(self.revision_date, (datetime, time_, int, float)):
            headers.append(('PO-Revision-Date', self.revision_date.strftime('%Y-%m-%d %H:%M%z')))
        else:
            headers.append(('PO-Revision-Date', self.revision_date))
        headers.append(('MIME-Version', '1.0'))
        headers.append(('Content-Type',
                        'text/plain; charset=%s' % self.charset))
        headers.append(('Content-Transfer-Encoding', '8bit'))
        headers.append(('Generated-By', 'Potter 1.0\n'))
        return headers

    def _set_mime_headers(self, headers):
        for name, value in headers:
            name = name.lower()
            if name == 'project-id-version':
                parts = value.split(' ')
                self.project = u' '.join(parts[:-1])
                self.version = parts[-1]
            elif name == 'report-msgid-bugs-to':
                self.msgid_bugs_address = value
            elif name == 'content-type':
                mimetype, params = parse_header(value)
                if 'charset' in params:
                    self.charset = params['charset'].lower()
            elif name == 'plural-forms':
                _, params = parse_header(' ;' + value)
                try:
                    self._num_plurals = int(params.get('nplurals', 2))
                except ValueError:
                    self._num_plurals = 2
                self._plural_expr = params.get('plural', '(n != 1)')
            elif name == 'pot-creation-date':
                # FIXME: this should use dates.parse_datetime as soon as that
                #        is ready
                value, tzoffset, _ = re.split('([+-]\d{4})$', value, 1)

                tt = time.strptime(value, '%Y-%m-%d %H:%M')
                ts = time.mktime(tt)

                # Separate the offset into a sign component, hours, and minutes
                plus_minus_s, rest = tzoffset[0], tzoffset[1:]
                hours_offset_s, mins_offset_s = rest[:2], rest[2:]

                # Make them all integers
                plus_minus = int(plus_minus_s + '1')
                hours_offset = int(hours_offset_s)
                mins_offset = int(mins_offset_s)

                # Calculate net offset
                net_mins_offset = hours_offset * 60
                net_mins_offset += mins_offset
                net_mins_offset *= plus_minus

                # Create an offset object
                tzoffset = FixedOffsetTimezone(net_mins_offset)

                # Store the offset in a datetime object
                dt = datetime.fromtimestamp(ts)
                self.creation_date = dt.replace(tzinfo=tzoffset)
            elif name == 'po-revision-date':
                # Keep the value if it's not the default one
                if 'YEAR' not in value:
                    # FIXME: this should use dates.parse_datetime as soon as
                    #        that is ready
                    value, tzoffset, _ = re.split('([+-]\d{4})$', value, 1)
                    tt = time.strptime(value, '%Y-%m-%d %H:%M')
                    ts = time.mktime(tt)

                    # Separate the offset into a sign component, hours, and
                    # minutes
                    plus_minus_s, rest = tzoffset[0], tzoffset[1:]
                    hours_offset_s, mins_offset_s = rest[:2], rest[2:]

                    # Make them all integers
                    plus_minus = int(plus_minus_s + '1')
                    hours_offset = int(hours_offset_s)
                    mins_offset = int(mins_offset_s)

                    # Calculate net offset
                    net_mins_offset = hours_offset * 60
                    net_mins_offset += mins_offset
                    net_mins_offset *= plus_minus

                    # Create an offset object
                    tzoffset = FixedOffsetTimezone(net_mins_offset)

                    # Store the offset in a datetime object
                    dt = datetime.fromtimestamp(ts)
                    self.revision_date = dt.replace(tzinfo=tzoffset)

    mime_headers = property(_get_mime_headers, _set_mime_headers, doc="""\
    The MIME headers of the catalog, used for the special ``msgid ""`` entry.

    Here's an example of the output for such a catalog template:

    >>> created = datetime(1990, 4, 1, 15, 30, tzinfo=UTC)
    >>> catalog = Catalog(project='Foobar', version='1.0',
    ...                   creation_date=created)
    >>> for name, value in catalog.mime_headers:
    ...     print '%s: %s' % (name, value)
    Project-Id-Version: Foobar 1.0
    Report-Msgid-Bugs-To: EMAIL@ADDRESS
    POT-Creation-Date: 1990-04-01 15:30+0000
    PO-Revision-Date: YEAR-MO-DA HO:MI+ZONE
    MIME-Version: 1.0
    Content-Type: text/plain; charset=utf-8
    Content-Transfer-Encoding: 8bit
    Generated-By: Potter ...

    :type: `list`
    """)

    def __contains__(self, id):
        """Return whether the catalog has a message with the specified ID."""
        return self._key_for(id) in self._messages

    def __len__(self):
        """The number of messages in the catalog.

        This does not include the special ``msgid ""`` entry."""
        return len(self._messages)

    def __iter__(self):
        """Iterates through all the entries in the catalog, in the order they
        were added, yielding a `Message` object for every entry.

        :rtype: ``iterator``"""
        buf = []
        for name, value in self.mime_headers:
            buf.append('%s: %s' % (name, value))
        yield Message(u'', '\n'.join(buf), flags=set())
        for key in self._messages:
            yield self._messages[key]

    def __repr__(self):
        return '<%s %r>' % (type(self).__name__, self.domain)

    def __delitem__(self, id):
        """Delete the message with the specified ID."""
        self.delete(id)

    def __getitem__(self, id):
        """Return the message with the specified ID.

        :param id: the message ID
        :return: the message with the specified ID, or `None` if no such
                 message is in the catalog
        :rtype: `Message`
        """
        return self.get(id)

    def __setitem__(self, id, message):
        """Add or update the message with the specified ID.

        >>> catalog = Catalog()
        >>> catalog[u'foo'] = Message(u'foo')
        >>> catalog[u'foo']
        <Message u'foo' (flags: [])>

        If a message with that ID is already in the catalog, it is updated
        to include the locations and flags of the new message.

        >>> catalog = Catalog()
        >>> catalog[u'foo'] = Message(u'foo', locations=[('main.py', 1)])
        >>> catalog[u'foo'].locations
        [('main.py', 1)]
        >>> catalog[u'foo'] = Message(u'foo', locations=[('utils.py', 5)])
        >>> catalog[u'foo'].locations
        [('main.py', 1), ('utils.py', 5)]

        :param id: the message ID
        :param message: the `Message` object
        """
        assert isinstance(message, Message), 'expected a Message object'
        key = self._key_for(id, message.context)
        current = self._messages.get(key)
        if current:
            if message.pluralizable and not current.pluralizable:
                # The new message adds pluralization
                current.id = message.id
                current.string = message.string
            current.locations = list(distinct(current.locations + message.locations))
            current.auto_comments = list(distinct(current.auto_comments + message.auto_comments))
            current.user_comments = list(distinct(current.user_comments + message.user_comments))
            current.flags |= message.flags
            message = current
        elif id == '':
            # special treatment for the header message
            def _parse_header(header_string):
                # message_from_string only works for str, not for unicode
                headers = message_from_string(header_string.encode('utf8'))
                decoded_headers = {}
                for name, value in headers.items():
                    name = name.decode('utf8')
                    value = value.decode('utf8')
                    decoded_headers[name] = value
                return decoded_headers
            self.mime_headers = _parse_header(message.string).items()
            self.header_comment = '\n'.join(['# %s' % comment for comment
                                             in message.user_comments])
        else:
            if isinstance(id, (list, tuple)):
                assert isinstance(message.string, (list, tuple)), \
                    'Expected sequence but got %s' % type(message.string)
            self._messages[key] = message

    def add(self, id, string=None, locations=(), flags=(), auto_comments=(),
            user_comments=(), previous_id=(), lineno=None, context=None, formatFlag=None):
        """Add or update the message with the specified ID.

        >>> catalog = Catalog()
        >>> catalog.add(u'foo')
        <Message ...>
        >>> catalog[u'foo']
        <Message u'foo' (flags: [])>

        This method simply constructs a `Message` object with the given
        arguments and invokes `__setitem__` with that object.

        :param id: the message ID, or a ``(singular, plural)`` tuple for
                   pluralizable messages
        :param string: the translated message string, or a
                       ``(singular, plural)`` tuple for pluralizable messages
        :param locations: a sequence of strings that determine where a message was found
        :param flags: a set or sequence of flags
        :param auto_comments: a sequence of automatic comments
        :param user_comments: a sequence of user comments
        :param previous_id: the previous message ID, or a ``(singular, plural)``
                            tuple for pluralizable messages
        :param lineno: the line number on which the msgid line was found in the
                       PO file, if any
        :param context: the message context
        :return: the newly added message
        :rtype: `Message`
        """
        message = Message(id, string, locations, flags, auto_comments,
                          user_comments, previous_id, lineno=lineno,
                          context=context, formatFlag=formatFlag)
        self[id] = message
        return message

    def get(self, id, context=None):
        """Return the message with the specified ID and context.

        :param id: the message ID
        :param context: the message context, or ``None`` for no context
        :return: the message with the specified ID, or `None` if no such
                 message is in the catalog
        :rtype: `Message`
        """
        return self._messages.get(self._key_for(id, context))

    def delete(self, id, context=None):
        """Delete the message with the specified ID and context.

        :param id: the message ID
        :param context: the message context, or ``None`` for no context
        """
        key = self._key_for(id, context)
        if key in self._messages:
            del self._messages[key]

    @property
    def num_plurals(self):
        if self._num_plurals is not None:
            return self._num_plurals
        else:
            return 2

    def _key_for(self, id, context=None):
        """The key for a message is just the singular ID even for pluralizable
        messages, but is a ``(msgid, msgctxt)`` tuple for context-specific
        messages.
        """
        key = id
        if isinstance(key, (list, tuple)):
            key = id[0]
        if context is not None:
            key = (key, context)
        return key
