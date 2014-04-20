# -*- coding: utf-8 -*-

VERSION = (0, 9, 0, 'final')


def get_version():
    version = '%s.%s' % (VERSION[0], VERSION[1])
    if VERSION[2]:
        version = '%s.%s' % (version, VERSION[2])
    if VERSION[3] != 'final':
        version = '%s %s' % (version, VERSION[3])
    return version
