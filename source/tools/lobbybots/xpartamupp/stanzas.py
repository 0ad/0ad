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

"""0ad-specific XMPP-stanzas."""

from sleekxmpp.xmlstream import ElementBase, ET


class BoardListXmppPlugin(ElementBase):
    """Class for custom boardlist and ratinglist stanza extension."""

    name = 'query'
    namespace = 'jabber:iq:boardlist'
    interfaces = {'board', 'command'}
    sub_interfaces = interfaces
    plugin_attrib = 'boardlist'

    def add_command(self, command):
        """Add a command to the extension.

        Arguments:
            command (str): Command to add
        """
        self.xml.append(ET.fromstring('<command>%s</command>' % command))

    def add_item(self, name, rating):
        """Add an item to the extension.

        Arguments:
            name (str): Name of the player to add
            rating (int): Rating of the player to add
        """
        self.xml.append(ET.Element('board', {'name': name, 'rating': str(rating)}))


class GameListXmppPlugin(ElementBase):
    """Class for custom gamelist stanza extension."""

    name = 'query'
    namespace = 'jabber:iq:gamelist'
    interfaces = {'game', 'command'}
    sub_interfaces = interfaces
    plugin_attrib = 'gamelist'

    def add_game(self, data):
        """Add a game to the extension.

        Arguments:
            data (dict): game data to add
        """
        try: del data['ip']	# Don't send the IP address with the gamelist.
        except: pass

        self.xml.append(ET.Element('game', data))

    def get_game(self):
        """Get game from stanza.

        Required to parse incoming stanzas with this extension.

        Returns:
            dict with game data

        """
        game = self.xml.find('{%s}game' % self.namespace)
        data = {}

        if game is not None:
            for key, item in game.items():
                data[key] = item
        return data


class GameReportXmppPlugin(ElementBase):
    """Class for custom gamereport stanza extension."""

    name = 'report'
    namespace = 'jabber:iq:gamereport'
    plugin_attrib = 'gamereport'
    interfaces = 'game'
    sub_interfaces = interfaces

    def add_game(self, game_report):
        """Add a game to the extension.

        Arguments:
            game_report (dict): a report about a game

        """
        self.xml.append(ET.fromstring(str(game_report)).find('{%s}game' % self.namespace))

    def get_game(self):
        """Get game from stanza.

        Required to parse incoming stanzas with this extension.

        Returns:
            dict with game information

        """
        game = self.xml.find('{%s}game' % self.namespace)
        data = {}

        if game is not None:
            for key, item in game.items():
                data[key] = item
        return data


class ProfileXmppPlugin(ElementBase):
    """Class for custom profile."""

    name = 'query'
    namespace = 'jabber:iq:profile'
    interfaces = {'profile', 'command'}
    sub_interfaces = interfaces
    plugin_attrib = 'profile'

    def add_command(self, player_nick):
        """Add a command to the extension.

        Arguments:
            player_nick (str): the nick of the player the profile is about

        """
        self.xml.append(ET.fromstring('<command>%s</command>' % player_nick))

    def add_item(self, player, rating, highest_rating=0,  # pylint: disable=too-many-arguments
                 rank=0, total_games_played=0, wins=0, losses=0):
        """Add an item to the extension.

        Arguments:
            player (str): Name of the player
            rating (int): Current rating of the player
            highest_rating (int): Highest rating the player had
            rank (int): Rank of the player
            total_games_played (int): Total number of games the player
                                      played
            wins (int): Number of won games the player had
            losses (int): Number of lost games the player had
        """
        item_xml = ET.Element('profile', {'player': player, 'rating': str(rating),
                                          'highestRating': str(highest_rating), 'rank': str(rank),
                                          'totalGamesPlayed': str(total_games_played),
                                          'wins': str(wins), 'losses': str(losses)})
        self.xml.append(item_xml)
