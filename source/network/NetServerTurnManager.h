/* Copyright (C) 2021 Wildfire Games.
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

#include "ps/CStr.h"

#include <map>
#include <unordered_map>

class CNetServerWorker;
class CNetServerSession;

/**
 * The server-side counterpart to CNetClientTurnManager.
 * Records the turn state of each client, and sends turn advancement messages
 * when clients are ready.
 *
 * Thread-safety:
 * - This is constructed and used by CNetServerWorker in the network server thread.
 */
class CNetServerTurnManager
{
	NONCOPYABLE(CNetServerTurnManager);
public:
	CNetServerTurnManager(CNetServerWorker& server);

	void NotifyFinishedClientCommands(CNetServerSession& session, u32 turn);

	void NotifyFinishedClientUpdate(CNetServerSession& session, u32 turn, const CStr& hash);

	/**
	 * Inform the turn manager of a new client
	 * @param observer - whether this client is an observer.
	 */
	void InitialiseClient(int client, u32 turn, bool observer);

	/**
	 * Inform the turn manager that a previously-initialised client has left the game.
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

	struct Client
	{
		CStrW playerName;
		// Latest turn for which all commands have been received.
		u32 readyTurn;
		// Last known simulated turn.
		u32 simulatedTurn;
		bool isObserver;
		bool isOOS = false;
	};

	std::unordered_map<int, Client> m_ClientsData;

	// Cached value - is any client OOS? This is reset when the OOS client leaves.
	bool m_HasSyncError = false;

	// Map of turn -> {Client ID -> state hash}; old indexes <= min(m_ClientsSimulated) are deleted
	std::map<u32, std::map<int, std::string>> m_ClientStateHashes;

	/// The latest turn for which we have received all commands from all clients
	u32 m_ReadyTurn;

	// Current turn length
	u32 m_TurnLength;

	// Turn lengths for all previously executed turns
	std::vector<u32> m_SavedTurnLengths;

	CNetServerWorker& m_NetServer;
};

#endif // INCLUDED_NETSERVERTURNMANAGER
