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

#ifndef INCLUDED_NETSERVERTURNMANAGER
#define INCLUDED_NETSERVERTURNMANAGER

#include <map>
#include "ps/CStr.h"

class CNetServerWorker;

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

	void NotifyFinishedClientUpdate(int client, const CStrW& playername, u32 turn, const CStr& hash);

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

private:
	void CheckClientsReady();

	/// The latest turn for which we have received all commands from all clients
	u32 m_ReadyTurn;

	// Client ID -> ready turn number (the latest turn for which all commands have been received from that client)
	std::map<int, u32> m_ClientsReady;

	// Client ID -> last known simulated turn number (for which we have the state hash)
	// (the client has reached the start of this turn, not done the update for it yet)
	std::map<int, u32> m_ClientsSimulated;

	// Map of turn -> {Client ID -> state hash}; old indexes <= min(m_ClientsSimulated) are deleted
	std::map<u32, std::map<int, std::string>> m_ClientStateHashes;

	// Map of client ID -> playername
	std::map<u32, CStrW> m_ClientPlayernames;

	// Current turn length
	u32 m_TurnLength;

	// Turn lengths for all previously executed turns
	std::vector<u32> m_SavedTurnLengths;

	CNetServerWorker& m_NetServer;

	bool m_HasSyncError;
};

#endif // INCLUDED_NETSERVERTURNMANAGER
