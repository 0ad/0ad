#!/usr/bin/env python3

import sqlalchemy
from sqlalchemy import Column, ForeignKey, Integer, String
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
	civ = String(20)
	economyScore = Column(Integer)
	militaryScore = Column(Integer)
	explorationScore = Column(Integer)
	# Store to avoid needlessly recomputing it.
	totalScore = Column(Integer)
	unitsTrained = Column(Integer)
	unitsLost = Column(Integer)
	unitsKilled = Column(Integer)
	buildingsConstructed = Column(Integer)
	buildingsLost = Column(Integer)
	buildingsDestroyed = Column(Integer)
	civCentersBuilt = Column(Integer)
	civCentersDestroyed = Column(Integer)
	percentMapExplored = Column(Integer)
	foodGathered = Column(Integer)
	foodUsed = Column(Integer)
	woodGathered = Column(Integer)
	woodUsed = Column(Integer)
	stoneGathered = Column(Integer)
	stoneUsed = Column(Integer)
	metalGathered = Column(Integer)
	metalUsed = Column(Integer)
	vegetarianRatio = Column(Integer)
	treasuresCollected = Column(Integer)
	tributesSent = Column(Integer)
	tributesRecieved = Column(Integer)
	foodBought = Column(Integer)
	foodSold = Column(Integer)
	woodBought = Column(Integer)
	woodSold = Column(Integer)
	stoneBought = Column(Integer)
	stoneSold = Column(Integer)
	metalBought = Column(Integer)
	metalSold = Column(Integer)
	barterEfficiency = Column(Integer)
	tradeIncome = Column(Integer)

class Game(Base):
	__tablename__ = 'games'

	id = Column(Integer, primary_key=True)
	map = Column(String(80))
	duration = Column(Integer)
	winner_id = Column(Integer, ForeignKey('players.id'))
	player_info = relationship('PlayerInfo', backref='game')
	players = relationship('Player', secondary='players_info')


if __name__ == '__main__':
	Base.metadata.create_all(engine)

