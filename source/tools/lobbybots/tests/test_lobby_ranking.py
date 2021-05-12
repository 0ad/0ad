# Copyright (C) 2021 Wildfire Games.
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

# pylint: disable=no-self-use

"""Tests for the database schema."""

import sys

from argparse import Namespace
from unittest import TestCase
from unittest.mock import Mock, patch

from parameterized import parameterized

from xpartamupp.lobby_ranking import main, parse_args


class TestArgumentParsing(TestCase):
    """Test handling of parsing command line parameters."""

    @parameterized.expand([
        (['create'], Namespace(action='create', database_url='sqlite:///lobby_rankings.sqlite3')),
        (['--database-url', 'sqlite:////tmp/db.sqlite3', 'create'],
         Namespace(action='create', database_url='sqlite:////tmp/db.sqlite3')),
    ])
    def test_valid(self, cmd_args, expected_args):
        """Test valid parameter combinations."""
        self.assertEqual(parse_args(cmd_args), expected_args)

    @parameterized.expand([
        ([],),
        (['--database-url=sqlite:////tmp/db.sqlite3'],),
    ])
    def test_missing_action(self, cmd_args):
        """Test invalid parameter combinations."""
        with self.assertRaises(SystemExit):
            parse_args(cmd_args)


class TestMain(TestCase):
    """Test main method."""

    def test_success(self):
        """Test successful execution."""
        with patch('xpartamupp.lobby_ranking.parse_args') as args_mock, \
                patch('xpartamupp.lobby_ranking.create_engine') as create_engine_mock, \
                patch('xpartamupp.lobby_ranking.Base') as declarative_base_mock:
            args_mock.return_value = Mock(action='create',
                                          database_url='sqlite:///lobby_rankings.sqlite3')
            engine_mock = Mock()
            create_engine_mock.return_value = engine_mock
            main()
            args_mock.assert_called_once_with(sys.argv[1:])
            create_engine_mock.assert_called_once_with(
                'sqlite:///lobby_rankings.sqlite3')
            declarative_base_mock.metadata.create_all.assert_any_call(engine_mock)
