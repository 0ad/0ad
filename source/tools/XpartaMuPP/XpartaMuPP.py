#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Copyright (C) 2014 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
"""

import logging, time, traceback
from optparse import OptionParser

import sleekxmpp
from sleekxmpp.stanza import Iq
from sleekxmpp.xmlstream import ElementBase, register_stanza_plugin, ET
from sleekxmpp.xmlstream.handler import Callback
from sleekxmpp.xmlstream.matcher import StanzaPath

from sqlalchemy import func

from LobbyRanking import session as db, Game, Player, PlayerInfo
from ELO import get_rating_adjustment
# Rating that new players should be inserted into the
# database with, before they've played any games.
leaderboard_default_rating = 1200

## Class that contains and manages leaderboard data ##
class LeaderboardList():
  def __init__(self, room):
    self.room = room
    self.lastRated = ""

  def getProfile(self, JID):
    """
      Retrieves the profile for the specified JID
    """
    stats = {}
    player = db.query(Player).filter(Player.jid.ilike(str(JID)))
    if not player.first():
      return
    if player.first().rating != -1:
      stats['rating'] = str(player.first().rating)

    if player.first().highest_rating != -1:
      stats['highestRating'] = str(player.first().highest_rating)

    playerID = player.first().id
    players = db.query(Player).filter(Player.rating != -1).order_by(Player.rating.desc()).all()

    for rank, user in enumerate(players):
      if (user.jid.lower() == JID.lower()):
        stats['rank'] = str(rank+1)
        break

    stats['totalGamesPlayed'] = str(db.query(PlayerInfo).filter_by(player_id=playerID).count())
    stats['wins'] = str(db.query(Game).filter_by(winner_id=playerID).count())
    stats['losses'] = str(db.query(PlayerInfo).filter_by(player_id=playerID).count() - db.query(Game).filter_by(winner_id=playerID).count())
    return stats

  def getOrCreatePlayer(self, JID):
    """
      Stores a player(JID) in the database if they don't yet exist.
      Returns either the newly created instance of
      the Player model, or the one that already
      exists in the database.
    """
    players = db.query(Player).filter_by(jid=str(JID))
    if not players.first():
      player = Player(jid=str(JID), rating=-1)
      db.add(player)
      db.commit()
      return player
    return players.first()

  def removePlayer(self, JID):
    """
      Remove a player(JID) from database.
      Returns the player that was removed, or None
      if that player didn't exist.
    """
    players = db.query(Player).filter_by(jid=JID)
    player = players.first()
    if not player:
      return None
    players.delete()
    return player

  def addGame(self, gamereport):
    """
      Adds a game to the database and updates the data
      on a player(JID) from game results.
      Returns the created Game object, or None if
      the creation failed for any reason.
      Side effects:
        Inserts a new Game instance into the database.
    """
    # Discard any games still in progress.
    if any(map(lambda state: state == 'active',
               dict.values(gamereport['playerStates']))):
      return None

    players = map(lambda jid: db.query(Player).filter(Player.jid.ilike(str(jid))).first(),
                  dict.keys(gamereport['playerStates']))

    winning_jid = list(dict.keys({jid: state for jid, state in
                                  gamereport['playerStates'].items()
                                  if state == 'won'}))[0]
    def get(stat, jid):
      return gamereport[stat][jid]

    singleStats = {'timeElapsed', 'mapName', 'teamsLocked', 'matchID'}
    totalScoreStats = {'economyScore', 'militaryScore', 'totalScore'}
    resourceStats = {'foodGathered', 'foodUsed', 'woodGathered', 'woodUsed', 
            'stoneGathered', 'stoneUsed', 'metalGathered', 'metalUsed', 'vegetarianFoodGathered',
            'treasuresCollected', 'lootCollected', 'tributesSent', 'tributesReceived'}
    unitsStats = {'totalUnitsTrained', 'totalUnitsLost', 'enemytotalUnitsKilled', 'infantryUnitsTrained',
            'infantryUnitsLost', 'enemyInfantryUnitsKilled', 'workerUnitsTrained', 'workerUnitsLost',
            'enemyWorkerUnitsKilled', 'femaleUnitsTrained', 'femaleUnitsLost', 'enemyFemaleUnitsKilled',
            'cavalryUnitsTrained', 'cavalryUnitsLost', 'enemyCavalryUnitsKilled', 'championUnitsTrained',
            'championUnitsLost', 'enemyChampionUnitsKilled', 'heroUnitsTrained', 'heroUnitsLost',
            'enemyHeroUnitsKilled', 'shipUnitsTrained', 'shipUnitsLost', 'enemyShipUnitsKilled', 'traderUnitsTrained',
            'traderUnitsLost', 'enemyTraderUnitsKilled'}
    buildingsStats = {'totalBuildingsConstructed', 'totalBuildingsLost', 'enemytotalBuildingsDestroyed',
            'civCentreBuildingsConstructed', 'civCentreBuildingsLost', 'enemyCivCentreBuildingsDestroyed',
            'houseBuildingsConstructed', 'houseBuildingsLost', 'enemyHouseBuildingsDestroyed',
            'economicBuildingsConstructed', 'economicBuildingsLost', 'enemyEconomicBuildingsDestroyed',
            'outpostBuildingsConstructed', 'outpostBuildingsLost', 'enemyOutpostBuildingsDestroyed',
            'militaryBuildingsConstructed', 'militaryBuildingsLost', 'enemyMilitaryBuildingsDestroyed',
            'fortressBuildingsConstructed', 'fortressBuildingsLost', 'enemyFortressBuildingsDestroyed',
            'wonderBuildingsConstructed', 'wonderBuildingsLost', 'enemyWonderBuildingsDestroyed'}
    marketStats = {'woodBought', 'foodBought', 'stoneBought', 'metalBought', 'tradeIncome'}
    miscStats = {'civs', 'teams', 'percentMapExplored'}

    stats = totalScoreStats | resourceStats | unitsStats | buildingsStats | marketStats | miscStats
    playerInfos = []
    for player in players:
      jid = player.jid
      playerinfo = PlayerInfo(player=player)
      for reportname in stats:
        setattr(playerinfo, reportname, get(reportname, jid.lower()))
      playerInfos.append(playerinfo)

    game = Game(map=gamereport['mapName'], duration=int(gamereport['timeElapsed']), teamsLocked=bool(gamereport['teamsLocked']), matchID=gamereport['matchID'])
    game.players.extend(players)
    game.player_info.extend(playerInfos)
    game.winner = db.query(Player).filter(Player.jid.ilike(str(winning_jid))).first()
    db.add(game)
    db.commit()
    return game
 
  def verifyGame(self, gamereport):
    """
      Returns a boolean based on whether the game should be rated.
      Here, we can specify the criteria for rated games.
    """
    winning_jids = list(dict.keys({jid: state for jid, state in
                                  gamereport['playerStates'].items()
                                  if state == 'won'}))
    # We only support 1v1s right now. TODO: Support team games.
    if len(winning_jids) * 2 > len(dict.keys(gamereport['playerStates'])):
      # More than half the people have won. This is not a balanced team game or duel.
      return False
    if len(dict.keys(gamereport['playerStates'])) != 2:
      return False
    return True

  def rateGame(self, game):
    """
      Takes a game with 2 players and alters their ratings
      based on the result of the game.
      Returns self.
      Side effects:
        Changes the game's players' ratings in the database.
    """
    player1 = game.players[0]
    player2 = game.players[1]
    # TODO: Support draws. Since it's impossible to draw in the game currently,
    # the database model, and therefore this code, requires a winner.
    # The Elo implementation does not, however.
    result = 1 if player1 == game.winner else -1
    # Player's ratings are -1 unless they have played a rated game.
    if player1.rating == -1:
      player1.rating = leaderboard_default_rating
    if player2.rating == -1:
      player2.rating = leaderboard_default_rating

    rating_adjustment1 = int(get_rating_adjustment(player1.rating, player2.rating,
      len(player1.games), len(player2.games), result))
    rating_adjustment2 = int(get_rating_adjustment(player2.rating, player1.rating,
      len(player2.games), len(player1.games), result * -1))
    if result == 1:
      resultQualitative = "won"
    elif result == 0:
      resultQualitative = "drew"
    else:
      resultQualitative = "lost"
    name1 = '@'.join(player1.jid.split('@')[:-1])
    name2 = '@'.join(player2.jid.split('@')[:-1])
    self.lastRated = "A rated game has ended. %s %s against %s. Rating Adjustment: %s (%s -> %s) and %s (%s -> %s)."%(name1, 
      resultQualitative, name2, name1, player1.rating, player1.rating + rating_adjustment1,
      name2, player2.rating, player2.rating + rating_adjustment2)
    player1.rating += rating_adjustment1
    player2.rating += rating_adjustment2
    if not player1.highest_rating:
      player1.highest_rating = -1
    if not player2.highest_rating:
      player2.highest_rating = -1
    if player1.rating > player1.highest_rating:
      player1.highest_rating = player1.rating
    if player2.rating > player2.highest_rating:
      player2.highest_rating = player2.rating
    db.commit()
    return self

  def getLastRatedMessage(self):
    """
      Gets the string of the last rated game. Triggers an update
      chat for the bot.
    """
    return self.lastRated

  def addAndRateGame(self, gamereport):
    """
      Calls addGame and if the game has only two
      players, also calls rateGame.
      Returns the result of addGame.
    """
    game = self.addGame(gamereport)
    if game and self.verifyGame(gamereport):
      self.rateGame(game)
    else:
      self.lastRated = ""
    return game

  def getBoard(self):
    """
      Returns a dictionary of player rankings to
        JIDs for sending.
    """
    board = {}
    players = db.query(Player).filter(Player.rating != -1).order_by(Player.rating.desc()).limit(100).all()
    for rank, player in enumerate(players):
      board[player.jid] = {'name': '@'.join(player.jid.split('@')[:-1]), 'rating': str(player.rating)}
    return board
  def getRatingList(self, nicks):
    """
    Returns a rating list of players
    currently in the lobby by nick
    because the client can't link
    JID to nick conveniently.
    """
    ratinglist = {}
    for JID in list(nicks):
      players = db.query(Player.jid, Player.rating).filter(Player.jid.ilike(str(JID)))
      if players.first():
        if players.first().rating == -1:
          ratinglist[nicks[JID]] = {'name': nicks[JID], 'rating': ''}
        else:
          ratinglist[nicks[JID]] = {'name': nicks[JID], 'rating': str(players.first().rating)}
      else:
        ratinglist[nicks[JID]] = {'name': nicks[JID], 'rating': ''}
    return ratinglist

## Class to tracks all games in the lobby ##
class GameList():
  def __init__(self):
    self.gameList = {}
  def addGame(self, JID, data):
    """
      Add a game
    """
    data['players-init'] = data['players']
    data['nbp-init'] = data['nbp']
    data['state'] = 'init'
    self.gameList[str(JID)] = data
  def removeGame(self, JID):
    """
      Remove a game attached to a JID
    """
    del self.gameList[str(JID)]
  def getAllGames(self):
    """
      Returns all games
    """
    return self.gameList
  def changeGameState(self, JID, data):
    """
      Switch game state between running and waiting
    """
    JID = str(JID)
    if JID in self.gameList:
      if self.gameList[JID]['nbp-init'] > data['nbp']:
        logging.debug("change game (%s) state from %s to %s", JID, self.gameList[JID]['state'], 'waiting')
        self.gameList[JID]['nbp'] = data['nbp']
        self.gameList[JID]['state'] = 'waiting'
      else:
        logging.debug("change game (%s) state from %s to %s", JID, self.gameList[JID]['state'], 'running')
        self.gameList[JID]['nbp'] = data['nbp']
        self.gameList[JID]['state'] = 'running'

## Class which manages different game reports from clients ##
##   and calls leaderboard functions as appropriate.       ##
class ReportManager():
  def __init__(self, leaderboard):
    self.leaderboard = leaderboard
    self.interimReportTracker = []
    self.interimJIDTracker = []

  def addReport(self, JID, rawGameReport):
    """
      Adds a game to the interface between a raw report
        and the leaderboard database.
    """
    # cleanRawGameReport is a copy of rawGameReport with all reporter specific information removed.
    cleanRawGameReport = rawGameReport.copy()
    del cleanRawGameReport["playerID"]

    if cleanRawGameReport not in self.interimReportTracker:
      # Store the game.
      appendIndex = len(self.interimReportTracker)
      self.interimReportTracker.append(cleanRawGameReport)
      # Initilize the JIDs and store the initial JID.
      numPlayers = self.getNumPlayers(rawGameReport)
      JIDs = [None] * numPlayers
      if numPlayers - int(rawGameReport["playerID"]) > -1:
        JIDs[int(rawGameReport["playerID"])-1] = str(JID).lower()
      self.interimJIDTracker.append(JIDs)
    else:
      # We get the index at which the JIDs coresponding to the game are stored.
      index = self.interimReportTracker.index(cleanRawGameReport)
      # We insert the new report JID into the acending list of JIDs for the game.
      JIDs = self.interimJIDTracker[index]
      if len(JIDs) - int(rawGameReport["playerID"]) > -1:
        JIDs[int(rawGameReport["playerID"])-1] = str(JID).lower()
      self.interimJIDTracker[index] = JIDs

    self.checkFull()

  def expandReport(self, rawGameReport, JIDs):
    """
      Takes an raw game report and re-formats it into
        Python data structures leaving JIDs empty.
      Returns a processed gameReport of type dict.
    """
    processedGameReport = {}
    for key in rawGameReport:
      if rawGameReport[key].find(",") == -1:
        processedGameReport[key] = rawGameReport[key]
      else:
        split = rawGameReport[key].split(",")
        # Remove the false split positive.
        split.pop()
        statToJID = {}
        for i, part in enumerate(split):
           statToJID[JIDs[i]] = part
        processedGameReport[key] = statToJID
    return processedGameReport

  def checkFull(self):
    """
      Searches internal database to check if enough
        reports have been submitted to add a game to
        the leaderboard. If so, the report will be
        interpolated and addAndRateGame will be
        called with the result.
    """
    i = 0
    length = len(self.interimReportTracker)
    while(i < length):
      numPlayers = self.getNumPlayers(self.interimReportTracker[i])
      numReports = 0
      for JID in self.interimJIDTracker[i]:
        if JID != None:
          numReports += 1
      if numReports == numPlayers:
        self.leaderboard.addAndRateGame(self.expandReport(self.interimReportTracker[i], self.interimJIDTracker[i]))
        del self.interimJIDTracker[i]
        del self.interimReportTracker[i]
        length -= 1
      else:
        i += 1
        self.leaderboard.lastRated = ""

  def getNumPlayers(self, rawGameReport):
    """
      Computes the number of players in a raw gameReport.
      Returns int, the number of players.
    """
    # Find a key in the report which holds values for multiple players.
    for key in rawGameReport:
      if rawGameReport[key].find(",") != -1:
        # Count the number of values, minus one for the false split positive.
        return len(rawGameReport[key].split(","))-1
    # Return -1 in case of failure.
    return -1

## Class for custom gamelist stanza extension ##
class GameListXmppPlugin(ElementBase):
  name = 'query'
  namespace = 'jabber:iq:gamelist'
  interfaces = set(('game', 'command'))
  sub_interfaces = interfaces
  plugin_attrib = 'gamelist'

  def addGame(self, data):
    itemXml = ET.Element("game", data)
    self.xml.append(itemXml)

  def getGame(self):
    """
      Required to parse incoming stanzas with this
        extension.
    """
    game = self.xml.find('{%s}game' % self.namespace)
    data = {}
    for key, item in game.items():
      data[key] = item
    return data

## Class for custom boardlist and ratinglist stanza extension ##
class BoardListXmppPlugin(ElementBase):
  name = 'query'
  namespace = 'jabber:iq:boardlist'
  interfaces = set(('board', 'command'))
  sub_interfaces = interfaces
  plugin_attrib = 'boardlist'
  def addCommand(self, command):
    commandXml = ET.fromstring("<command>%s</command>" % command)
    self.xml.append(commandXml)
  def addItem(self, name, rating):
    itemXml = ET.Element("board", {"name": name, "rating": rating})
    self.xml.append(itemXml)

## Class for custom gamereport stanza extension ##
class GameReportXmppPlugin(ElementBase):
  name = 'report'
  namespace = 'jabber:iq:gamereport'
  plugin_attrib = 'gamereport'
  interfaces = ('game')
  sub_interfaces = interfaces

  def getGame(self):
    """
      Required to parse incoming stanzas with this
        extension.
    """
    game = self.xml.find('{%s}game' % self.namespace)
    data = {}
    for key, item in game.items():
      data[key] = item
    return data

## Class for custom profile ##
class ProfileXmppPlugin(ElementBase):
  name = 'query'
  namespace = 'jabber:iq:profile'
  interfaces = set(('profile', 'command'))
  sub_interfaces = interfaces
  plugin_attrib = 'profile'
  def addCommand(self, command):
    commandXml = ET.fromstring("<command>%s</command>" % command)
    self.xml.append(commandXml)
  def addItem(self, player, rating, highestRating, rank, totalGamesPlayed, wins, losses):
    itemXml = ET.Element("profile", {"player": player, "rating": rating, "highestRating": highestRating,
                                      "rank" : rank, "totalGamesPlayed" : totalGamesPlayed, "wins" : wins,
                                      "losses" : losses})
    self.xml.append(itemXml)

## Main class which handles IQ data and sends new data ##
class XpartaMuPP(sleekxmpp.ClientXMPP):
  """
  A simple list provider
  """
  def __init__(self, sjid, password, room, nick):
    sleekxmpp.ClientXMPP.__init__(self, sjid, password)
    self.sjid = sjid
    self.room = room
    self.nick = nick

    self.ratingListUpdate = False
    # Game collection
    self.gameList = GameList()

    # Init leaderboard object
    self.leaderboard = LeaderboardList(room)

    # gameReport to leaderboard abstraction
    self.reportManager = ReportManager(self.leaderboard)

    # Store mapping of nicks and XmppIDs, attached via presence stanza
    self.nicks = {}

    self.lastLeft = ""

    register_stanza_plugin(Iq, GameListXmppPlugin)
    register_stanza_plugin(Iq, BoardListXmppPlugin)
    register_stanza_plugin(Iq, GameReportXmppPlugin)
    register_stanza_plugin(Iq, ProfileXmppPlugin)

    self.register_handler(Callback('Iq Gamelist',
                                       StanzaPath('iq/gamelist'),
                                       self.iqhandler,
                                       instream=True))
    self.register_handler(Callback('Iq Boardlist',
                                       StanzaPath('iq/boardlist'),
                                       self.iqhandler,
                                       instream=True))
    self.register_handler(Callback('Iq GameReport',
                                       StanzaPath('iq/gamereport'),
                                       self.iqhandler,
                                       instream=True))
                                       
    self.register_handler(Callback('Iq Profile',
                                       StanzaPath('iq/profile'),
                                       self.iqhandler,
                                       instream=True))

    self.add_event_handler("session_start", self.start)
    self.add_event_handler("muc::%s::got_online" % self.room, self.muc_online)
    self.add_event_handler("muc::%s::got_offline" % self.room, self.muc_offline)
    self.add_event_handler("groupchat_message", self.muc_message)

  def start(self, event):
    """
    Process the session_start event
    """
    self.plugin['xep_0045'].joinMUC(self.room, self.nick)
    self.send_presence()
    self.get_roster()
    logging.info("XpartaMuPP started")

  def muc_online(self, presence):
    """
    Process presence stanza from a chat room.
    """
    self.ratingListUpdate = True
    if presence['muc']['nick'] != self.nick:
      # If it doesn't already exist, store player JID mapped to their nick.
      if str(presence['muc']['jid']) not in self.nicks:
        self.nicks[str(presence['muc']['jid'])] = presence['muc']['nick']
      # Check the jid isn't already in the lobby.
      # Send Gamelist to new player.
      self.sendGameList(presence['muc']['jid'])
      # Following two calls make sqlalchemy complain about using objects in the
      #  incorrect thread. TODO: Figure out how to fix this.
      # Send Leaderboard to new player.
      #self.sendBoardList(presence['muc']['jid'])
      # Register on leaderboard.
      #self.leaderboard.getOrCreatePlayer(presence['muc']['jid'])
      logging.debug("Client '%s' connected with a nick of '%s'." %(presence['muc']['jid'], presence['muc']['nick']))

  def muc_offline(self, presence):
    """
    Process presence stanza from a chat room.
    """
    # Clean up after a player leaves
    if presence['muc']['nick'] != self.nick:
      # Delete any games they were hosting.
      for JID in self.gameList.getAllGames():
        if JID == str(presence['muc']['jid']):
          self.gameList.removeGame(JID)
          self.sendGameList()
          break
      # Remove them from the local player list.
      self.lastLeft = str(presence['muc']['jid'])
      if str(presence['muc']['jid']) in self.nicks:
        del self.nicks[str(presence['muc']['jid'])]

  def muc_message(self, msg):
    """
    Process new messages from the chatroom.
    """
    if msg['mucnick'] != self.nick and self.nick.lower() in msg['body'].lower():
      self.send_message(mto=msg['from'].bare,
                        mbody="I am the administrative bot in this lobby and cannot participate in any games.",
                        mtype='groupchat')

  def iqhandler(self, iq):
    """
    Handle the custom stanzas
      This method should be very robust because we could receive anything
    """
    if iq['type'] == 'error':
      logging.error('iqhandler error' + iq['error']['condition'])
      #self.disconnect()
    elif iq['type'] == 'get':
      """
      Request lists.
      """
      # Send lists/register on leaderboard; depreciated once muc_online
      #  can send lists/register automatically on joining the room.
      if 'gamelist' in iq.plugins:
        try:
          self.sendGameList(iq['from'])
        except:
          traceback.print_exc()
          logging.error("Failed to process gamelist request from %s" % iq['from'].bare)
      elif 'boardlist' in iq.plugins:
        command = iq['boardlist']['command']
        if command == 'getleaderboard':
          try:
            self.leaderboard.getOrCreatePlayer(iq['from'])
            self.sendBoardList(iq['from'])
          except:
            traceback.print_exc()
            logging.error("Failed to process leaderboardlist request from %s" % iq['from'].bare)
        else:
          logging.error("Failed to process boardlist request from %s" % iq['from'].bare)
      elif 'profile' in iq.plugins:
        command = iq['profile']['command']
        try:
          self.sendProfile(iq['from'], command)
        except:
          try:
            self.sendProfileNotFound(iq['from'], command)
          except:
            logging.debug("No record found for %s" % command)
      else:
        logging.error("Unknown 'get' type stanza request from %s" % iq['from'].bare)
    elif iq['type'] == 'result':
      """
      Iq successfully received
      """
      pass
    elif iq['type'] == 'set':
      if 'gamelist' in iq.plugins:
        """
        Register-update / unregister a game
        """
        command = iq['gamelist']['command']
        if command == 'register':
          # Add game
          try:
            self.gameList.addGame(iq['from'], iq['gamelist']['game'])
            self.sendGameList()
          except:
            traceback.print_exc()
            logging.error("Failed to process game registration data")
        elif command == 'unregister':
          # Remove game
          try:
            self.gameList.removeGame(iq['from'])
            self.sendGameList()
          except:
            traceback.print_exc()
            logging.error("Failed to process game unregistration data")

        elif command == 'changestate':
          # Change game status (waiting/running)
          try:
            self.gameList.changeGameState(iq['from'], iq['gamelist']['game'])
            self.sendGameList()
          except:
            traceback.print_exc()
            logging.error("Failed to process changestate data")
        else:
          logging.error("Failed to process command '%s' received from %s" % command, iq['from'].bare)
      elif 'gamereport' in iq.plugins:
        """
        Client is reporting end of game statistics
        """
        try:
          self.reportManager.addReport(iq['from'], iq['gamereport']['game'])
          if self.leaderboard.getLastRatedMessage() != "":
            self.send_message(mto=self.room, mbody=self.leaderboard.getLastRatedMessage(), mtype="groupchat",
              mnick=self.nick)
            self.sendBoardList()
            self.sendRatingList()
        except:
          traceback.print_exc()
          logging.error("Failed to update game statistics for %s" % iq['from'].bare)
    else:
       logging.error("Failed to process stanza type '%s' received from %s" % iq['type'], iq['from'].bare)
    if self.ratingListUpdate == True:
      self.sendRatingList()
      self.ratingListUpdate = False

  def sendGameList(self, to = ""):
    """
      Send a massive stanza with the whole game list.
      If no target is passed the gamelist is broadcasted
        to all clients.
    """
    games = self.gameList.getAllGames()
    if to == "":
      for JID in list(self.nicks):
        stz = GameListXmppPlugin()

        ## Pull games and add each to the stanza        
        for JIDs in games:
          g = games[JIDs]
          stz.addGame(g)

        ## Set additional IQ attributes
        iq = self.Iq()
        iq['type'] = 'result'
        iq['to'] = JID
        iq.setPayload(stz)

        ## Try sending the stanza
        try:
          iq.send(block=False, now=True)
        except:
          logging.error("Failed to send game list")
    else:
      ## Check recipient exists
      if str(to) not in self.nicks:
        logging.error("No player with the XmPP ID '%s' known to send gamelist to." % str(to))
        return
      stz = GameListXmppPlugin()

      ## Pull games and add each to the stanza
      for JIDs in games:
        g = games[JIDs]
        stz.addGame(g)

      ## Set additional IQ attributes
      iq = self.Iq()
      iq['type'] = 'result'
      iq['to'] = to
      iq.setPayload(stz)

      ## Try sending the stanza
      try:
        iq.send(block=False, now=True)
      except:
        logging.error("Failed to send game list")

  def sendBoardList(self, to = ""):
    """
      Send the whole leaderboard list.
      If no target is passed the boardlist is broadcasted
        to all clients.
    """
    ## Pull leaderboard data and add it to the stanza  
    board = self.leaderboard.getBoard()
    stz = BoardListXmppPlugin()
    iq = self.Iq()
    iq['type'] = 'result'
    for i in board:
      stz.addItem(board[i]['name'], board[i]['rating'])
    stz.addCommand('boardlist')
    iq.setPayload(stz)
    if to == "":    
      for JID in list(self.nicks):
        ## Set additional IQ attributes
        iq['to'] = JID
        ## Try sending the stanza
        try:
          iq.send(block=False, now=True)
        except:
          logging.error("Failed to send leaderboard list")
    else:
      ## Check recipient exists
      if str(to) not in self.nicks:
        logging.error("No player with the XmPP ID '%s' known to send boardlist to" % str(to))
        return
      ## Set additional IQ attributes
      iq['to'] = to
      ## Try sending the stanza
      try:
        iq.send(block=False, now=True)
      except:
        logging.error("Failed to send leaderboard list")

  def sendRatingList(self, to = ""):
    """
      Send the rating list.
      If no target is passed the rating list is broadcasted
        to all clients.
    """
    ## Pull rating list data and add it to the stanza  
    ratinglist = self.leaderboard.getRatingList(self.nicks)
    stz = BoardListXmppPlugin()
    iq = self.Iq()
    iq['type'] = 'result'
    for i in ratinglist:
      stz.addItem(ratinglist[i]['name'], ratinglist[i]['rating'])
    stz.addCommand('ratinglist')
    iq.setPayload(stz)
    if to == "":    
      for JID in list(self.nicks):
        ## Set additional IQ attributes
        iq['to'] = JID
        ## Try sending the stanza
        try:
          iq.send(block=False, now=True)
        except:
          logging.error("Failed to send rating list")
    else:
      ## Check recipient exists
      if str(to) not in self.nicks:
        logging.error("No player with the XmPP ID '%s' known to send ratinglist to" % str(to))
        return
      ## Set additional IQ attributes
      iq['to'] = to
      ## Try sending the stanza
      try:
        iq.send(block=False, now=True)
      except:
        logging.error("Failed to send rating list")

  def sendProfile(self, to, player):
    """
      Send the profile to a specified player.
    """
    if to == "":
      logging.error("Failed to send profile")
      return

    online = False;
    ## Pull stats and add it to the stanza
    for JID in list(self.nicks):
      if self.nicks[JID] == player:
        stats = self.leaderboard.getProfile(JID)
        online = True
        break

    if online == False:
      stats = self.leaderboard.getProfile(player + "@" + str(to).split('@')[1])
    stz = ProfileXmppPlugin()
    iq = self.Iq()
    iq['type'] = 'result'

    stz.addItem(player, stats['rating'], stats['highestRating'], stats['rank'], stats['totalGamesPlayed'], stats['wins'], stats['losses'])
    stz.addCommand(player)
    iq.setPayload(stz)
    ## Check recipient exists
    if str(to) not in self.nicks:
      logging.error("No player with the XmPP ID '%s' known to send profile to" % str(to))
      return

    ## Set additional IQ attributes
    iq['to'] = to

    ## Try sending the stanza
    try:
      iq.send(block=False, now=True)
    except:
      traceback.print_exc()
      logging.error("Failed to send profile")

  def sendProfileNotFound(self, to, player):
    """
      Send a profile not-found error to a specified player.
    """
    stz = ProfileXmppPlugin()
    iq = self.Iq()
    iq['type'] = 'result'

    filler = str(0)
    stz.addItem(player, str(-2), filler, filler, filler, filler, filler)
    stz.addCommand(player)
    iq.setPayload(stz)
    ## Check recipient exists
    if str(to) not in self.nicks:
      logging.error("No player with the XmPP ID '%s' known to send profile to" % str(to))
      return

    ## Set additional IQ attributes
    iq['to'] = to

    ## Try sending the stanza
    try:
      iq.send(block=False, now=True)
    except:
      traceback.print_exc()
      logging.error("Failed to send profile")

## Main Program ##
if __name__ == '__main__':
  # Setup the command line arguments.
  optp = OptionParser()

  # Output verbosity options.
  optp.add_option('-q', '--quiet', help='set logging to ERROR',
                  action='store_const', dest='loglevel',
                  const=logging.ERROR, default=logging.INFO)
  optp.add_option('-d', '--debug', help='set logging to DEBUG',
                  action='store_const', dest='loglevel',
                  const=logging.DEBUG, default=logging.INFO)
  optp.add_option('-v', '--verbose', help='set logging to COMM',
                  action='store_const', dest='loglevel',
                  const=5, default=logging.INFO)

  # XpartaMuPP configuration options
  optp.add_option('-m', '--domain', help='set xpartamupp domain',
                  action='store', dest='xdomain',
                  default="lobby.wildfiregames.com")
  optp.add_option('-l', '--login', help='set xpartamupp login',
                  action='store', dest='xlogin',
                  default="xpartamupp")
  optp.add_option('-p', '--password', help='set xpartamupp password',
                  action='store', dest='xpassword',
                  default="XXXXXX")
  optp.add_option('-n', '--nickname', help='set xpartamupp nickname',
                  action='store', dest='xnickname',
                  default="WFGbot")
  optp.add_option('-r', '--room', help='set muc room to join',
                  action='store', dest='xroom',
                  default="arena")

  opts, args = optp.parse_args()

  # Setup logging.
  logging.basicConfig(level=opts.loglevel,
                      format='%(asctime)s        %(levelname)-8s %(message)s', datefmt='%Y-%m-%d %H:%M:%S')

  # XpartaMuPP
  xmpp = XpartaMuPP(opts.xlogin+'@'+opts.xdomain+'/CC', opts.xpassword, opts.xroom+'@conference.'+opts.xdomain, opts.xnickname)
  xmpp.register_plugin('xep_0030') # Service Discovery
  xmpp.register_plugin('xep_0004') # Data Forms
  xmpp.register_plugin('xep_0045') # Multi-User Chat	# used
  xmpp.register_plugin('xep_0060') # PubSub
  xmpp.register_plugin('xep_0199') # XMPP Ping

  if xmpp.connect():
    xmpp.process(threaded=False)
  else:
    logging.error("Unable to connect")
