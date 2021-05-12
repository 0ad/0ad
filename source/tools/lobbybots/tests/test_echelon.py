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

"""Tests for EcheLOn."""

import sys

from argparse import Namespace
from unittest import TestCase
from unittest.mock import Mock, call, patch

from parameterized import parameterized
from sleekxmpp.jid import JID
from sqlalchemy import create_engine

from xpartamupp.echelon import main, parse_args, Leaderboard
from xpartamupp.lobby_ranking import Base


class TestLeaderboard(TestCase):
    """Test Leaderboard functionality."""

    def setUp(self):
        """Set up a leaderboard instance."""
        db_url = 'sqlite://'
        engine = create_engine(db_url)
        Base.metadata.create_all(engine)
        with patch('xpartamupp.echelon.create_engine') as create_engine_mock:
            create_engine_mock.return_value = engine
            self.leaderboard = Leaderboard(db_url)

    def test_create_player(self):
        """Test creating a new player."""
        player = self.leaderboard.get_or_create_player(JID('john@localhost'))
        self.assertEqual(player.id, 1)
        self.assertEqual(player.jid, 'john@localhost')
        self.assertEqual(player.rating, -1)
        self.assertEqual(player.highest_rating, None)
        self.assertEqual(player.games, [])
        self.assertEqual(player.games_info, [])
        self.assertEqual(player.games_won, [])

    def test_get_profile_no_player(self):
        """Test profile retrieval fro not existing player."""
        profile = self.leaderboard.get_profile(JID('john@localhost'))
        self.assertEqual(profile, dict())

    def test_get_profile_player_without_games(self):
        """Test profile retrieval for existing player."""
        self.leaderboard.get_or_create_player(JID('john@localhost'))
        profile = self.leaderboard.get_profile(JID('john@localhost'))
        self.assertDictEqual(profile, {'highestRating': None, 'losses': 0, 'totalGamesPlayed': 0,
                                       'wins': 0})


class TestReportManager(TestCase):
    """Test ReportManager functionality."""

    pass


class TestArgumentParsing(TestCase):
    """Test handling of parsing command line parameters."""

    @parameterized.expand([
        ([], Namespace(domain='lobby.wildfiregames.com', login='EcheLOn', log_level=30, xserver=None, xdisabletls=False,
                       nickname='RatingsBot', password='XXXXXX', room='arena',
                       database_url='sqlite:///lobby_rankings.sqlite3')),
        (['--debug'],
         Namespace(domain='lobby.wildfiregames.com', login='EcheLOn', log_level=10, xserver=None,xdisabletls=False,
                   nickname='RatingsBot', password='XXXXXX', room='arena',
                   database_url='sqlite:///lobby_rankings.sqlite3')),
        (['--quiet'],
         Namespace(domain='lobby.wildfiregames.com', login='EcheLOn', log_level=40, xserver=None,xdisabletls=False,
                   nickname='RatingsBot', password='XXXXXX', room='arena',
                   database_url='sqlite:///lobby_rankings.sqlite3')),
        (['--verbose'],
         Namespace(domain='lobby.wildfiregames.com', login='EcheLOn', log_level=20, xserver=None, xdisabletls=False,
                   nickname='RatingsBot', password='XXXXXX', room='arena',
                   database_url='sqlite:///lobby_rankings.sqlite3')),
        (['-m', 'lobby.domain.tld'],
         Namespace(domain='lobby.domain.tld', login='EcheLOn', log_level=30, nickname='RatingsBot', xserver=None, xdisabletls=False,
                   password='XXXXXX', room='arena',
                   database_url='sqlite:///lobby_rankings.sqlite3')),
        (['--domain=lobby.domain.tld'],
         Namespace(domain='lobby.domain.tld', login='EcheLOn', log_level=30, nickname='RatingsBot', xserver=None, xdisabletls=False,
                   password='XXXXXX', room='arena',
                   database_url='sqlite:///lobby_rankings.sqlite3')),
        (['-m' 'lobby.domain.tld', '-l', 'bot', '-p', '123456', '-n', 'Bot', '-r', 'arena123',
          '-v'],
         Namespace(domain='lobby.domain.tld', login='bot', log_level=20, nickname='Bot', xserver=None, xdisabletls=False,
                   password='123456', room='arena123',
                   database_url='sqlite:///lobby_rankings.sqlite3')),
        (['--domain=lobby.domain.tld', '--login=bot', '--password=123456', '--nickname=Bot',
          '--room=arena123', '--database-url=sqlite:////tmp/db.sqlite3', '--verbose'],
         Namespace(domain='lobby.domain.tld', login='bot', log_level=20, nickname='Bot', xserver=None, xdisabletls=False,
                   password='123456', room='arena123',
                   database_url='sqlite:////tmp/db.sqlite3')),
    ])
    def test_valid(self, cmd_args, expected_args):
        """Test valid parameter combinations."""
        self.assertEqual(parse_args(cmd_args), expected_args)

    @parameterized.expand([
        (['-f'],),
        (['--foo'],),
        (['--debug', '--quiet'],),
        (['--quiet', '--verbose'],),
        (['--debug', '--verbose'],),
        (['--debug', '--quiet', '--verbose'],),
    ])
    def test_invalid(self, cmd_args):
        """Test invalid parameter combinations."""
        with self.assertRaises(SystemExit):
            parse_args(cmd_args)


class TestMain(TestCase):
    """Test main method."""

    def test_success(self):
        """Test successful execution."""
        with patch('xpartamupp.echelon.parse_args') as args_mock, \
                patch('xpartamupp.echelon.Leaderboard') as leaderboard_mock, \
                patch('xpartamupp.echelon.EcheLOn') as xmpp_mock:
            args_mock.return_value = Mock(log_level=30, login='EcheLOn',
                                          domain='lobby.wildfiregames.com', password='XXXXXX',
                                          room='arena', nickname='RatingsBot',
                                          database_url='sqlite:///lobby_rankings.sqlite3',
                                          xserver=None, xdisabletls=False)
            main()
            args_mock.assert_called_once_with(sys.argv[1:])
            leaderboard_mock.assert_called_once_with('sqlite:///lobby_rankings.sqlite3')
            xmpp_mock().register_plugin.assert_has_calls([call('xep_0004'), call('xep_0030'),
                                                          call('xep_0045'), call('xep_0060'),
                                                          call('xep_0199', {'keepalive': True})],
                                                         any_order=True)
            xmpp_mock().connect.assert_called_once_with(None, True, True)
            xmpp_mock().process.assert_called_once_with()

    def test_failing_connect(self):
        """Test failing connect to XMPP server."""
        with patch('xpartamupp.echelon.parse_args') as args_mock, \
                patch('xpartamupp.echelon.Leaderboard') as leaderboard_mock, \
                patch('xpartamupp.echelon.EcheLOn') as xmpp_mock:
            args_mock.return_value = Mock(log_level=30, login='EcheLOn',
                                          domain='lobby.wildfiregames.com', password='XXXXXX',
                                          room='arena', nickname='RatingsBot',
                                          database_url='sqlite:///lobby_rankings.sqlite3',
                                          xserver=None, xdisabletls=False)

            xmpp_mock().connect.return_value = False
            main()
            args_mock.assert_called_once_with(sys.argv[1:])
            leaderboard_mock.assert_called_once_with('sqlite:///lobby_rankings.sqlite3')
            xmpp_mock().register_plugin.assert_has_calls([call('xep_0004'), call('xep_0030'),
                                                          call('xep_0045'), call('xep_0060'),
                                                          call('xep_0199', {'keepalive': True})],
                                                         any_order=True)
            xmpp_mock().connect.assert_called_once_with(None, True, True)
            xmpp_mock().process.assert_not_called()
