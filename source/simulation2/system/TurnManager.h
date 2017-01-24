/* Copyright (C) 2017 Wildfire Games.
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

#ifndef INCLUDED_TURNMANAGER
#define INCLUDED_TURNMANAGER

#include "simulation2/helpers/SimulationCommand.h"

#include <list>
#include <map>
#include <vector>

extern const u32 DEFAULT_TURN_LENGTH_SP;
extern const u32 DEFAULT_TURN_LENGTH_MP;

extern const int COMMAND_DELAY;

class CSimulationMessage;
class CSimulation2;
class IReplayLogger;

/**
 * This file defines the base class of the turn managers for clients, local games and replays.
 * The basic idea of our turn managing system across a network is as in this article:
 * http://www.gamasutra.com/view/feature/3094/1500_archers_on_a_288_network_.php?print=1
 *
 * Each player performs the simulation for turn N.
 * User input is translated into commands scheduled for execution in turn N+2 which are
 * distributed to all other clients.
 * After a while, a client wants to perform the simulation for turn N+1,
 * which first requires that it has all the other clients' commands for turn N+1.
 * In that case, it does the simulation and tells all the other clients (via the server)
 * it has finished sending commands for turn N+2, and it starts sending commands for turn N+3.
 *
 * Commands are redistributed immediately by the server.
 * To ensure a consistent execution of commands, they are each associated with a
 * client session ID (which is globally unique and consistent), which is used to sort them.
 */

/**
 * Common turn system (used by clients and offline games).
 */
class CTurnManager
{
	NONCOPYABLE(CTurnManager);
public:
	/**
	 * Construct for a given network session ID.
	 */
	CTurnManager(CSimulation2& simulation, u32 defaultTurnLength, int clientId, IReplayLogger& replay);

	virtual ~CTurnManager() { }

	void ResetState(u32 newCurrentTurn, u32 newReadyTurn);

	/**
	 * Set the current user's player ID, which will be added into command messages.
	 */
	void SetPlayerID(int playerId);

	/**
	 * Advance the simulation by a certain time. If this brings us past the current
	 * turn length, the next turns are processed and the function returns true.
	 * Otherwise, nothing happens and it returns false.
	 *
	 * @param simFrameLength Length of the previous frame, in simulation seconds
	 * @param maxTurns Maximum number of turns to simulate at once
	 */
	bool Update(float simFrameLength, size_t maxTurns);

	/**
	 * Advance the simulation by as much as possible. Intended for catching up
	 * over a small number of turns when rejoining a multiplayer match.
	 * Returns true if it advanced by at least one turn.
	 */
	bool UpdateFastForward();

	/**
	 * Returns whether Update(simFrameLength, ...) will process at least one new turn.
	 * @param simFrameLength Length of the previous frame, in simulation seconds
	 */
	bool WillUpdate(float simFrameLength) const;

	/**
	 * Advance the graphics by a certain time.
	 * @param simFrameLength Length of the previous frame, in simulation seconds
	 * @param realFrameLength Length of the previous frame, in real time seconds
	 */
	void Interpolate(float simFrameLength, float realFrameLength);

	/**
	 * Called by networking code when a simulation message is received.
	 */
	virtual void OnSimulationMessage(CSimulationMessage* msg) = 0;

	/**
	 * Called by simulation code, to add a new command to be distributed to all clients and executed soon.
	 */
	virtual void PostCommand(JS::HandleValue data) = 0;

	/**
	 * Called when all commands for a given turn have been received.
	 * This allows Update to progress to that turn.
	 */
	void FinishedAllCommands(u32 turn, u32 turnLength);

	/**
	 * Enables the recording of state snapshots every @p numTurns,
	 * which can be jumped back to via RewindTimeWarp().
	 * If @p numTurns is 0 then recording is disabled.
	 */
	void EnableTimeWarpRecording(size_t numTurns);

	/**
	 * Jumps back to the latest recorded state snapshot (if any).
	 */
	void RewindTimeWarp();

	void QuickSave();
	void QuickLoad();

	u32 GetCurrentTurn() { return m_CurrentTurn; }

protected:
	/**
	 * Store a command to be executed at a given turn.
	 */
	void AddCommand(int client, int player, JS::HandleValue data, u32 turn);

	/**
	 * Called when this client has finished sending all its commands scheduled for the given turn.
	 */
	virtual void NotifyFinishedOwnCommands(u32 turn) = 0;

	/**
	 * Called when this client has finished a simulation update.
	 */
	virtual void NotifyFinishedUpdate(u32 turn) = 0;

	/**
	 * Returns whether we should compute a complete state hash for the given turn,
	 * instead of a quick less-complete hash.
	 */
	bool TurnNeedsFullHash(u32 turn) const;

	CSimulation2& m_Simulation2;

	/// The turn that we have most recently executed
	u32 m_CurrentTurn;

	/// The latest turn for which we have received all commands from all clients
	u32 m_ReadyTurn;

	// Current turn length
	u32 m_TurnLength;

	/// Commands queued at each turn (index 0 is for m_CurrentTurn+1)
	std::deque<std::map<u32, std::vector<SimulationCommand>>> m_QueuedCommands;

	int m_PlayerId;
	uint m_ClientId;

	/// Simulation time remaining until we ought to execute the next turn (as a negative value to
	/// add elapsed time increments to until we reach 0).
	float m_DeltaSimTime;

	bool m_HasSyncError;

	IReplayLogger& m_Replay;

	// The number of the last turn that is allowed to be executed (used for replays)
	u32 m_FinalTurn;

private:
	size_t m_TimeWarpNumTurns; // 0 if disabled
	std::list<std::string> m_TimeWarpStates;
	std::string m_QuickSaveState; // TODO: should implement a proper disk-based quicksave system
	std::string m_QuickSaveMetadata;
};

#endif // INCLUDED_TURNMANAGER
