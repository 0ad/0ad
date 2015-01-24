/* Copyright (C) 2012 Wildfire Games.
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

#ifndef INCLUDED_NETTURNMANAGER
#define INCLUDED_NETTURNMANAGER

#include "simulation2/helpers/SimulationCommand.h"

#include <list>
#include <map>

class CNetServerWorker;
class CNetClient;
class CSimulationMessage;
class CSimulation2;
class IReplayLogger;

/*
 * This file deals with the logic of the network turn system. The basic idea is as in
 * http://www.gamasutra.com/view/feature/3094/1500_archers_on_a_288_network_.php?print=1
 *
 * Each player performs the simulation for turn N.
 * User input is translated into commands scheduled for execution in turn N+2 which are
 * distributed to all other clients.
 * After a while, a player wants to perform the simulation for turn N+1,
 * which first requires that it has all the other clients' commands for turn N+1.
 * In that case, it does the simulation and tells all the other clients (via the server)
 * it has finished sending commands for turn N+2, and it starts sending commands for turn N+3.
 *
 * Commands are redistributed immediately by the server.
 * To ensure a consistent execution of commands, they are each associated with a
 * client session ID (which is globally unique and consistent), which is used to sort them.
 */

/**
 * Common network turn system (used by clients and offline games).
 */
class CNetTurnManager
{
	NONCOPYABLE(CNetTurnManager);
public:
	/**
	 * Construct for a given network session ID.
	 */
	CNetTurnManager(CSimulation2& simulation, u32 defaultTurnLength, int clientId, IReplayLogger& replay);

	virtual ~CNetTurnManager() { }

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
	bool WillUpdate(float simFrameLength);

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
	 * Called when there has been an out-of-sync error.
	 */
	virtual void OnSyncError(u32 turn, const std::string& expectedHash);

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
	bool TurnNeedsFullHash(u32 turn);

	CSimulation2& m_Simulation2;

	/// The turn that we have most recently executed
	u32 m_CurrentTurn;

	/// The latest turn for which we have received all commands from all clients
	u32 m_ReadyTurn;

	// Current turn length
	u32 m_TurnLength;

	/// Commands queued at each turn (index 0 is for m_CurrentTurn+1)
	std::deque<std::map<u32, std::vector<SimulationCommand> > > m_QueuedCommands;

	int m_PlayerId;
	uint m_ClientId;

	/// Simulation time remaining until we ought to execute the next turn (as a negative value to
	/// add elapsed time increments to until we reach 0).
	float m_DeltaSimTime;

	bool m_HasSyncError;

	IReplayLogger& m_Replay;

private:
	size_t m_TimeWarpNumTurns; // 0 if disabled
	std::list<std::string> m_TimeWarpStates;
	std::string m_QuickSaveState; // TODO: should implement a proper disk-based quicksave system
	std::string m_QuickSaveMetadata;
};

/**
 * Implementation of CNetTurnManager for network clients.
 */
class CNetClientTurnManager : public CNetTurnManager
{
public:
	CNetClientTurnManager(CSimulation2& simulation, CNetClient& client, int clientId, IReplayLogger& replay);

	virtual void OnSimulationMessage(CSimulationMessage* msg);

	virtual void PostCommand(JS::HandleValue data);
	
	/**
	 * Notifiy the server that all commands are sent to prepare the connection for termination.
	 */
	void OnDestroyConnection();

protected:
	virtual void NotifyFinishedOwnCommands(u32 turn);

	virtual void NotifyFinishedUpdate(u32 turn);

	CNetClient& m_NetClient;
};

/**
 * Implementation of CNetTurnManager for offline games.
 */
class CNetLocalTurnManager : public CNetTurnManager
{
public:
	CNetLocalTurnManager(CSimulation2& simulation, IReplayLogger& replay);

	virtual void OnSimulationMessage(CSimulationMessage* msg);

	virtual void PostCommand(JS::HandleValue data);

protected:
	virtual void NotifyFinishedOwnCommands(u32 turn);

	virtual void NotifyFinishedUpdate(u32 turn);
};


/**
 * The server-side counterpart to CNetClientTurnManager.
 * Records the turn state of each client, and sends turn advancement messages
 * when all clients are ready.
 *
 * Thread-safety:
 * - This is constructed and used by CNetServerWorker in the network server thread.
 */
class CNetServerTurnManager
{
	NONCOPYABLE(CNetServerTurnManager);
public:
	CNetServerTurnManager(CNetServerWorker& server);

	void NotifyFinishedClientCommands(int client, u32 turn);

	void NotifyFinishedClientUpdate(int client, u32 turn, const std::string& hash);

	/**
	 * Inform the turn manager of a new client who will be sending commands.
	 */
	void InitialiseClient(int client, u32 turn);

	/**
	 * Inform the turn manager that a previously-initialised client has left the game
	 * and will no longer be sending commands.
	 */
	void UninitialiseClient(int client);

	void SetTurnLength(u32 msecs);

	/**
	 * Returns the latest turn for which all clients are ready;
	 * they will have already been told to execute this turn.
	 */
	u32 GetReadyTurn() { return m_ReadyTurn; }

	/**
	 * Returns the turn length that was used for the given turn.
	 * Requires turn <= GetReadyTurn().
	 */
	u32 GetSavedTurnLength(u32 turn);

protected:
	void CheckClientsReady();

	/// The latest turn for which we have received all commands from all clients
	u32 m_ReadyTurn;

	// Client ID -> ready turn number (the latest turn for which all commands have been received from that client)
	std::map<int, u32> m_ClientsReady;

	// Client ID -> last known simulated turn number (for which we have the state hash)
	// (the client has reached the start of this turn, not done the update for it yet)
	std::map<int, u32> m_ClientsSimulated;

	// Map of turn -> {Client ID -> state hash}; old indexes <= min(m_ClientsSimulated) are deleted
	std::map<u32, std::map<int, std::string> > m_ClientStateHashes;

	// Current turn length
	u32 m_TurnLength;

	// Turn lengths for all previously executed turns
	std::vector<u32> m_SavedTurnLengths;

	CNetServerWorker& m_NetServer;
};

#endif // INCLUDED_NETTURNMANAGER
