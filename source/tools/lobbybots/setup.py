#!/usr/bin/env python3

"""setup.py for 0ad XMPP lobby bots."""

from setuptools import find_packages, setup

setup(
    name='XpartaMuPP',
    version='0.24',
    description='Multiplayer lobby bots for 0ad',
    packages=find_packages(),
    entry_points={
        'console_scripts': [
            'echelon=xpartamupp.echelon:main',
            'xpartamupp=xpartamupp.xpartamupp:main',
            'echelon-db=xpartamupp.lobby_ranking:main',
        ]
    },
    install_requires=[
        'dnspython',
        'sleekxmpp',
        'sqlalchemy',
    ],
    tests_require=[
        'coverage',
        'hypothesis',
        'parameterized',
    ],
    classifiers=[
        'Development Status :: 3 - Alpha',
        'License :: OSI Approved :: GNU General Public License v2 or later (GPLv2+)',
        'Operating System :: OS Independent',
        'Programming Language :: Python',
        'Programming Language :: Python :: 3.4',
        'Programming Language :: Python :: 3.5',
        'Programming Language :: Python :: 3.6',
        'Topic :: Games/Entertainment',
        'Topic :: Internet :: XMPP',
    ],
    zip_safe=False,
    test_suite='tests',
)
