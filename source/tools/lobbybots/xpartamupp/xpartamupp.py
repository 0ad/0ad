#!/usr/bin/env python3
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

"""0ad XMPP-bot responsible for managing game listings."""

import argparse
import logging
import time
import sys

import sleekxmpp
from sleekxmpp.stanza import Iq
from sleekxmpp.xmlstream.handler import Callback
from sleekxmpp.xmlstream.matcher import StanzaPath
from sleekxmpp.xmlstream.stanzabase import register_stanza_plugin

from xpartamupp.stanzas import GameListXmppPlugin
from xpartamupp.utils import LimitedSizeDict


class Games(object):
    """Class to tracks all games in the lobby."""

    def __init__(self):
        """Initialize with empty games."""
        self.games = LimitedSizeDict(size_limit=2**7)

    def add_game(self, jid, data):
        """Add a game.

        Arguments:
            jid (sleekxmpp.jid.JID): JID of the player who started the
                game
            data (dict): information about the game

        Returns:
            True if adding the game succeeded, False if not

        """
        try:
            data['players-init'] = data['players']
            data['nbp-init'] = data['nbp']
            data['state'] = 'init'
        except (KeyError, TypeError, ValueError):
            logging.warning("Received invalid data for add game from 0ad: %s", data)
            return False
        else:
            self.games[jid] = data
            return True

    def remove_game(self, jid):
        """Remove a game attached to a JID.

        Arguments:
            jid (sleekxmpp.jid.JID): JID of the player whose game to
                remove.

        Returns:
            True if removing the game succeeded, False if not

        """
        try:
            del self.games[jid]
        except KeyError:
            logging.warning("Game for jid %s didn't exist", jid)
            return False
        else:
            return True

    def get_all_games(self):
        """Return all games.

        Returns:
            dict containing all games with the JID of the player who
            started the game as key.

        """
        return self.games

    def change_game_state(self, jid, data):
        """Switch game state between running and waiting.

        Arguments:
            jid (sleekxmpp.jid.JID): JID of the player whose game to
                change
            data (dict): information about the game

        Returns:
            True if changing the game state succeeded, False if not

        """
        if jid not in self.games:
            logging.warning("Tried to change state for non-existent game %s", jid)
            return False

        try:
            if self.games[jid]['nbp-init'] > data['nbp']:
                logging.debug("change game (%s) state from %s to %s", jid,
                              self.games[jid]['state'], 'waiting')
                self.games[jid]['state'] = 'waiting'
            else:
                logging.debug("change game (%s) state from %s to %s", jid,
                              self.games[jid]['state'], 'running')
                self.games[jid]['state'] = 'running'
            self.games[jid]['nbp'] = data['nbp']
            self.games[jid]['players'] = data['players']
        except (KeyError, ValueError):
            logging.warning("Received invalid data for change game state from 0ad: %s", data)
            return False
        else:
            if 'startTime' not in self.games[jid]:
                self.games[jid]['startTime'] = str(round(time.time()))
            return True


class XpartaMuPP(sleekxmpp.ClientXMPP):
    """Main class which handles IQ data and sends new data."""

    def __init__(self, sjid, password, room, nick):
        """Initialize XpartaMuPP.

        Arguments:
             sjid (sleekxmpp.jid.JID): JID to use for authentication
             password (str): password to use for authentication
             room (str): XMPP MUC room to join
             nick (str): Nick to use in MUC

        """
        sleekxmpp.ClientXMPP.__init__(self, sjid, password)
        self.whitespace_keepalive = False

        self.room = room
        self.nick = nick

        self.games = Games()

        register_stanza_plugin(Iq, GameListXmppPlugin)

        self.register_handler(Callback('Iq Gamelist', StanzaPath('iq@type=set/gamelist'),
                                       self._iq_game_list_handler))

        self.add_event_handler('session_start', self._session_start)
        self.add_event_handler('muc::%s::got_online' % self.room, self._muc_online)
        self.add_event_handler('muc::%s::got_offline' % self.room, self._muc_offline)
        self.add_event_handler('groupchat_message', self._muc_message)

    def _session_start(self, event):  # pylint: disable=unused-argument
        """Join MUC channel and announce presence.

        Arguments:
            event (dict): empty dummy dict

        """
        self.plugin['xep_0045'].joinMUC(self.room, self.nick)
        self.send_presence()
        self.get_roster()
        logging.info("XpartaMuPP started")

    def _muc_online(self, presence):
        """Add joining players to the list of players.

        Also send a list of games to them, so they see which games
        are currently there.

        Arguments:
            presence (sleekxmpp.stanza.presence.Presence): Received
                presence stanza.

        """
        nick = str(presence['muc']['nick'])
        jid = sleekxmpp.jid.JID(presence['muc']['jid'])

        if nick == self.nick:
            return

        if jid.resource not in ['0ad', 'CC']:
            return

        self._send_game_list(jid)

        logging.debug("Client '%s' connected with a nick '%s'.", jid, nick)

    def _muc_offline(self, presence):
        """Remove leaving players from the list of players.

        Also remove the potential game this player was hosting, so we
        don't end up with stale games.

        Arguments:
            presence (sleekxmpp.stanza.presence.Presence): Received
                presence stanza.

        """
        nick = str(presence['muc']['nick'])
        jid = sleekxmpp.jid.JID(presence['muc']['jid'])

        if nick == self.nick:
            return

        if self.games.remove_game(jid):
            self._send_game_list()

        logging.debug("Client '%s' with nick '%s' disconnected", jid, nick)

    def _muc_message(self, msg):
        """Process messages in the MUC room.

        Respond to messages highlighting the bots name with an
        informative message.

        Arguments:
            msg (sleekxmpp.stanza.message.Message): Received MUC
                message
        """
        if msg['mucnick'] != self.nick and self.nick.lower() in msg['body'].lower():
            self.send_message(mto=msg['from'].bare,
                              mbody="I am just a bot and I'm responsible to ensure that your're"
                                    "able to see the list of games in here. Aside from that I'm"
                                    "just chilling.",
                              mtype='groupchat')

    def _iq_game_list_handler(self, iq):
        """Handle game state change requests.

        Arguments:
            iq (sleekxmpp.stanza.iq.IQ): Received IQ stanza

        """
        if iq['from'].resource != '0ad':
            return

        command = iq['gamelist']['command']
        if command == 'register':
            success = self.games.add_game(iq['from'], iq['gamelist']['game'])
        elif command == 'unregister':
            success = self.games.remove_game(iq['from'])
        elif command == 'changestate':
            success = self.games.change_game_state(iq['from'], iq['gamelist']['game'])
        else:
            logging.info('Received unknown game command: "%s"', command)
            return

        if success:
            try:
                self._send_game_list()
            except Exception:
                logging.exception('Failed to send game list after "%s" command', command)

    def _send_game_list(self, to=None):
        """Send a massive stanza with the whole game list.

        If no target is passed the gamelist is broadcasted to all
        clients.

        Arguments:
            to (sleekxmpp.jid.JID): Player to send the game list to.
                If None, the game list will be broadcasted
        """
        games = self.games.get_all_games()

        stanza = GameListXmppPlugin()
        for jid in games:
            stanza.add_game(games[jid])

        if not to:
            for nick in self.plugin['xep_0045'].getRoster(self.room):
                if nick == self.nick:
                    continue
                jid_str = self.plugin['xep_0045'].getJidProperty(self.room, nick, 'jid')
                jid = sleekxmpp.jid.JID(jid_str)
                iq = self.make_iq_result(ito=jid)
                iq.set_payload(stanza)
                try:
                    iq.send(block=False)
                except Exception:
                    logging.exception("Failed to send game list to %s", jid)
        else:
            iq = self.make_iq_result(ito=to)
            iq.set_payload(stanza)
            try:
                iq.send(block=False)
            except Exception:
                logging.exception("Failed to send game list to %s", to)


def parse_args(args):
    """Parse command line arguments.

    Arguments:
        args (dict): Raw command line arguments given to the script

    Returns:
         Parsed command line arguments

    """
    parser = argparse.ArgumentParser(formatter_class=argparse.ArgumentDefaultsHelpFormatter,
                                     description="XpartaMuPP - XMPP Multiplayer Game Manager")

    log_settings = parser.add_mutually_exclusive_group()
    log_settings.add_argument('-q', '--quiet', help="only log errors", action='store_const',
                              dest='log_level', const=logging.ERROR)
    log_settings.add_argument('-d', '--debug', help="log debug messages", action='store_const',
                              dest='log_level', const=logging.DEBUG)
    log_settings.add_argument('-v', '--verbose', help="log more informative messages",
                              action='store_const', dest='log_level', const=logging.INFO)
    log_settings.set_defaults(log_level=logging.WARNING)

    parser.add_argument('-m', '--domain', help="XMPP server to connect to",
                        default='lobby.wildfiregames.com')
    parser.add_argument('-l', '--login', help="username for login", default='xpartamupp')
    parser.add_argument('-p', '--password', help="password for login", default='XXXXXX')
    parser.add_argument('-n', '--nickname', help="nickname shown to players", default='WFGBot')
    parser.add_argument('-r', '--room', help="XMPP MUC room to join", default='arena')
    parser.add_argument('-s', '--server', help='address of the ejabberd server',
                  action='store', dest='xserver', default=None)
    parser.add_argument('-t', '--disable-tls', help='Pass this argument to connect without TLS encryption',
                  action='store_true', dest='xdisabletls', default=False)

    return parser.parse_args(args)


def main():
    """Entry point a console script."""
    args = parse_args(sys.argv[1:])

    logging.basicConfig(level=args.log_level,
                        format='%(asctime)s %(levelname)-8s %(message)s',
                        datefmt='%Y-%m-%d %H:%M:%S')

    xmpp = XpartaMuPP(sleekxmpp.jid.JID('%s@%s/%s' % (args.login, args.domain, 'CC')),
                      args.password, args.room + '@conference.' + args.domain, args.nickname)
    xmpp.register_plugin('xep_0030')  # Service Discovery
    xmpp.register_plugin('xep_0004')  # Data Forms
    xmpp.register_plugin('xep_0045')  # Multi-User Chat
    xmpp.register_plugin('xep_0060')  # Publish-Subscribe
    xmpp.register_plugin('xep_0199', {'keepalive': True})  # XMPP Ping

    if xmpp.connect((args.xserver, 5222) if args.xserver else None, True, not args.xdisabletls):
      xmpp.process()
    else:
      logging.error("Unable to connect")


if __name__ == '__main__':
    main()
