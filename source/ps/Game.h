/* Copyright (C) 2013 Wildfire Games.
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

#ifndef INCLUDED_GAME
#define INCLUDED_GAME

#include "ps/Errors.h"
#include <vector>

#include "scriptinterface/ScriptVal.h"

class CWorld;
class CSimulation2;
class CGameView;
class CNetTurnManager;
class IReplayLogger;
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
	 * Timescale multiplier for simulation rate.
	 **/
	float m_SimRate;

	int m_PlayerID;

	CNetTurnManager* m_TurnManager;

public:
	CGame(bool disableGraphics = false);
	~CGame();

	/**
	 * the game is paused and no updates will be performed if true.
	 **/
	bool m_Paused;

	void StartGame(const CScriptValRooted& attribs, const std::string& savedState);
	PSRETURN ReallyStartGame();

	/**
	 * Periodic heartbeat that controls the process. performs all per-frame updates.
	 * Simulation update is called and game status update is called.
	 *
	 * @param deltaRealTime Elapsed real time since last beat/frame, in seconds.
	 * @param doInterpolate Perform graphics interpolation if true.
	 * @return bool false if it can't keep up with the desired simulation rate
	 *	indicating that you might want to render less frequently.
	 */
	bool Update(const double deltaRealTime, bool doInterpolate = true);

	void Interpolate(float simFrameLength, float realFrameLength);

	int GetPlayerID();
	void SetPlayerID(int playerID);

	/**
	 * Retrieving player colours from scripts is slow, so this updates an
	 * internal cache of all players' colours.
	 * Call this just before rendering, so it will always have the latest
	 * colours.
	 */
	void CachePlayerColours();

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
	{	if (isfinite(simRate)) m_SimRate = std::max(simRate, 0.0f); }

	inline float GetSimRate()
	{	return m_SimRate; }

	/**
	 * Replace the current turn manager.
	 * This class will take ownership of the pointer.
	 */
	void SetTurnManager(CNetTurnManager* turnManager);

	CNetTurnManager* GetTurnManager() const
	{	return m_TurnManager; }

	IReplayLogger& GetReplayLogger() const
	{	return *m_ReplayLogger; }

private:
	void RegisterInit(const JS::HandleValue attribs, const std::string& savedState);
	IReplayLogger* m_ReplayLogger;

	std::vector<CColor> m_PlayerColours;

	int LoadInitialState();
	std::string m_InitialSavedState; // valid between RegisterInit and LoadInitialState
	bool m_IsSavedGame; // true if loading a saved game; false for a new game
};

extern CGame *g_Game;

#endif
