/* Copyright (C) 2010 Wildfire Games.
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
 */

/**
 * File        : Game.h
 * Project     : engine
 * Description : Contains the CGame Class which is a representation of the game itself.
 *
 **/
#ifndef INCLUDED_GAME
#define INCLUDED_GAME

#include "ps/Errors.h"
#include <vector>

class CWorld;
class CSimulation2;
class CGameView;
class CNetTurnManager;
class CScriptValRooted;
struct CColor;

/**
 * The container that holds the rules, resources and attributes of the game.
 * The CGame object is responsible for creating a game that is defined by
 * a set of attributes provided. The CGame object is also responsible for
 * maintaining the relations between CPlayer and CWorld, CSimulation and CWorld.
 **/
class CGame
{
	NONCOPYABLE(CGame);
	/**
	 * pointer to the CWorld object representing the game world.
	 **/
	CWorld *m_World;
	/**
	 * pointer to the CSimulation2 object operating on the game world.
	 **/
	CSimulation2 *m_Simulation2;
	/**
	 * pointer to the CGameView object representing the view into the game world.
	 **/
	CGameView *m_GameView;
	/**
	 * the game has been initialized and ready for use if true.
	 **/
	bool m_GameStarted;
	/**
	 * scale multiplier for simulation rate.
	 **/
	float m_SimRate;

	int m_PlayerID;

	CNetTurnManager* m_TurnManager;

public:
	enum ENetStatus
	{
		NET_WAITING_FOR_CONNECT, /// we have loaded the game; waiting for other players to finish loading
		NET_NORMAL /// running the game
	};

	CGame(bool disableGraphics = false);
	~CGame();

	/**
	 * the game is paused and no updates will be performed if true.
	 **/
	bool m_Paused;

	void StartGame(const CScriptValRooted& attribs);
	PSRETURN ReallyStartGame();

	/*
		Perform all per-frame updates
	*/
	bool Update(double deltaTime, bool doInterpolate = true);

	void Interpolate(float frameLength);

	int GetPlayerID();
	void SetPlayerID(int playerID);

	CColor GetPlayerColour(int player) const;

	/**
	 * Get m_GameStarted.
	 *
	 * @return bool the value of m_GameStarted.
	 **/
	inline bool IsGameStarted() const
	{
		return m_GameStarted;
	}

	/**
	 * Get the pointer to the game world object.
	 *
	 * @return CWorld * the value of m_World.
	 **/
	inline CWorld *GetWorld()
	{	return m_World; }

	/**
	 * Get the pointer to the game view object.
	 *
	 * @return CGameView * the value of m_GameView.
	 **/
	inline CGameView *GetView()
	{	return m_GameView; }

	/**
	 * Get the pointer to the simulation2 object.
	 *
	 * @return CSimulation2 * the value of m_Simulation2.
	 **/
	inline CSimulation2 *GetSimulation2()
	{	return m_Simulation2; }

	/**
	 * Set the simulation scale multiplier.
	 *
	 * @param simRate Float value to set m_SimRate to.
	 *						Because m_SimRate is also used to
	 *						scale TimeSinceLastFrame it must be
	 *						clamped to 0.0f.
	 **/
	inline void SetSimRate(float simRate)
	{	 m_SimRate = std::max(simRate, 0.0f); }

	/**
	 * Replace the current turn manager.
	 * This class will take ownership of the pointer.
	 */
	void SetTurnManager(CNetTurnManager* turnManager);

	CNetTurnManager* GetTurnManager() const
	{	return m_TurnManager; }

private:
	void RegisterInit(const CScriptValRooted& attribs);
};

extern CGame *g_Game;

#endif
