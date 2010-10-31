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

#ifndef INCLUDED_NETTURNMANAGER
#define INCLUDED_NETTURNMANAGER

#include "simulation2/helpers/SimulationCommand.h"

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

	/**
	 * Set the current user's player ID, which will be added into command messages.
	 */
	void SetPlayerID(int playerId);

	/**
	 * Advance the simulation by a certain time. If this brings us past the current
	 * turn length, the next turn is processed and the function returns true.
	 * Otherwise, nothing happens and it returns false.
	 * @param frameLength length of the previous frame in seconds
	 */
	bool Update(float frameLength);

	/**
	 * Advance the graphics by a certain time.
	 * @param frameLength length of the previous frame in seconds
	 */
	void Interpolate(float frameLength);

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
	virtual void PostCommand(CScriptValRooted data) = 0;

	/**
	 * Called when all commands for a given turn have been received.
	 * This allows Update to progress to that turn.
	 */
	void FinishedAllCommands(u32 turn, u32 turnLength);

protected:
	/**
	 * Store a command to be executed at a given turn.
	 */
	void AddCommand(int client, int player, CScriptValRooted data, u32 turn);

	/**
	 * Called when this client has finished sending all its commands scheduled for the given turn.
	 */
	virtual void NotifyFinishedOwnCommands(u32 turn) = 0;

	/**
	 * Called when this client has finished a simulation update.
	 */
	virtual void NotifyFinishedUpdate(u32 turn) = 0;

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

	/// Time remaining until we ought to execute the next turn
	float m_DeltaTime;

	bool m_HasSyncError;

	IReplayLogger& m_Replay;
};

/**
 * Implementation of CNetTurnManager for network clients.
 */
class CNetClientTurnManager : public CNetTurnManager
{
public:
	CNetClientTurnManager(CSimulation2& simulation, CNetClient& client, int clientId, IReplayLogger& replay);

	virtual void OnSimulationMessage(CSimulationMessage* msg);

	virtual void PostCommand(CScriptValRooted data);

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

	virtual void PostCommand(CScriptValRooted data);

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

	void InitialiseClient(int client);

	void SetTurnLength(u32 msecs);

protected:
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

	CNetServerWorker& m_NetServer;
};

#endif // INCLUDED_NETTURNMANAGER
