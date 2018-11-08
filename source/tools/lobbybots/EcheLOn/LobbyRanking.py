#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Copyright (C) 2013 Wildfire Games.
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

import sqlalchemy
from sqlalchemy import Column, ForeignKey, Integer, String, Boolean
from sqlalchemy.orm import relationship, sessionmaker
from sqlalchemy.ext.declarative import declarative_base

engine = sqlalchemy.create_engine('sqlite:///lobby_rankings.sqlite3')
Session = sessionmaker(bind=engine)
session = Session()
Base = declarative_base()

class Player(Base):
	__tablename__ = 'players'

	id = Column(Integer, primary_key=True)
	jid = Column(String(255))
	rating = Column(Integer)
	highest_rating = Column(Integer)
	games = relationship('Game', secondary='players_info')
	# These two relations really only exist to satisfy the linkage
	# between PlayerInfo and Player and Game and player.
	games_info = relationship('PlayerInfo', backref='player')
	games_won = relationship('Game', backref='winner')

class PlayerInfo(Base):
	__tablename__ = 'players_info'

	id = Column(Integer, primary_key=True)
	player_id = Column(Integer, ForeignKey('players.id'))
	game_id = Column(Integer, ForeignKey('games.id'))
	civs = Column(String(20))
	teams = Column(Integer)
	economyScore = Column(Integer)
	militaryScore = Column(Integer)
	totalScore = Column(Integer)
	foodGathered = Column(Integer)
	foodUsed = Column(Integer)
	woodGathered = Column(Integer)
	woodUsed = Column(Integer)
	stoneGathered = Column(Integer)
	stoneUsed = Column(Integer)
	metalGathered = Column(Integer)
	metalUsed = Column(Integer)
	vegetarianFoodGathered = Column(Integer)
	treasuresCollected = Column(Integer)
	lootCollected = Column(Integer)
	tributesSent = Column(Integer)
	tributesReceived = Column(Integer)
	totalUnitsTrained = Column(Integer)
	totalUnitsLost = Column(Integer)
	enemytotalUnitsKilled = Column(Integer)
	infantryUnitsTrained = Column(Integer)
	infantryUnitsLost = Column(Integer)
	enemyInfantryUnitsKilled = Column(Integer)
	workerUnitsTrained = Column(Integer)
	workerUnitsLost = Column(Integer)
	enemyWorkerUnitsKilled = Column(Integer)
	femaleCitizenUnitsTrained = Column(Integer)
	femaleCitizenUnitsLost = Column(Integer)
	enemyFemaleCitizenUnitsKilled = Column(Integer)
	cavalryUnitsTrained = Column(Integer)
	cavalryUnitsLost = Column(Integer)
	enemyCavalryUnitsKilled = Column(Integer)
	championUnitsTrained = Column(Integer)
	championUnitsLost = Column(Integer)
	enemyChampionUnitsKilled = Column(Integer)
	heroUnitsTrained = Column(Integer)
	heroUnitsLost = Column(Integer)
	enemyHeroUnitsKilled = Column(Integer)
	shipUnitsTrained = Column(Integer)
	shipUnitsLost = Column(Integer)
	enemyShipUnitsKilled = Column(Integer)
	traderUnitsTrained = Column(Integer)
	traderUnitsLost = Column(Integer)
	enemyTraderUnitsKilled = Column(Integer)
	totalBuildingsConstructed = Column(Integer)
	totalBuildingsLost = Column(Integer)
	enemytotalBuildingsDestroyed = Column(Integer)
	civCentreBuildingsConstructed = Column(Integer)
	civCentreBuildingsLost = Column(Integer)
	enemyCivCentreBuildingsDestroyed = Column(Integer)
	houseBuildingsConstructed = Column(Integer)
	houseBuildingsLost = Column(Integer)
	enemyHouseBuildingsDestroyed = Column(Integer)
	economicBuildingsConstructed = Column(Integer)
	economicBuildingsLost = Column(Integer)
	enemyEconomicBuildingsDestroyed = Column(Integer)
	outpostBuildingsConstructed = Column(Integer)
	outpostBuildingsLost = Column(Integer)
	enemyOutpostBuildingsDestroyed = Column(Integer)
	militaryBuildingsConstructed = Column(Integer)
	militaryBuildingsLost = Column(Integer)
	enemyMilitaryBuildingsDestroyed = Column(Integer)
	fortressBuildingsConstructed = Column(Integer)
	fortressBuildingsLost = Column(Integer)
	enemyFortressBuildingsDestroyed = Column(Integer)
	wonderBuildingsConstructed = Column(Integer)
	wonderBuildingsLost = Column(Integer)
	enemyWonderBuildingsDestroyed = Column(Integer)
	woodBought = Column(Integer)
	foodBought = Column(Integer)
	stoneBought = Column(Integer)
	metalBought = Column(Integer)
	tradeIncome = Column(Integer)
	percentMapExplored = Column(Integer)

class Game(Base):
	__tablename__ = 'games'

	id = Column(Integer, primary_key=True)
	map = Column(String(80))
	duration = Column(Integer)
	teamsLocked = Column(Boolean)
	matchID = Column(String(20))
	winner_id = Column(Integer, ForeignKey('players.id'))
	player_info = relationship('PlayerInfo', backref='game')
	players = relationship('Player', secondary='players_info')


if __name__ == '__main__':
	Base.metadata.create_all(engine)

