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

#include "precompiled.h"

#include "NetMessage.h"
#include "NetServerTurnManager.h"
#include "NetServer.h"

#include "simulation2/system/TurnManager.h"

#if 0
#define NETSERVERTURN_LOG(...) debug_printf(__VA_ARGS__)
#else
#define NETSERVERTURN_LOG(...)
#endif

CNetServerTurnManager::CNetServerTurnManager(CNetServerWorker& server)
	: m_NetServer(server), m_ReadyTurn(1), m_TurnLength(DEFAULT_TURN_LENGTH_MP), m_HasSyncError(false)
{
	// The first turn we will actually execute is number 2,
	// so store dummy values into the saved lengths list
	m_SavedTurnLengths.push_back(0);
	m_SavedTurnLengths.push_back(0);
}

void CNetServerTurnManager::NotifyFinishedClientCommands(int client, u32 turn)
{
	NETSERVERTURN_LOG("NotifyFinishedClientCommands(client=%d, turn=%d)\n", client, turn);

	// Must be a client we've already heard of
	ENSURE(m_ClientsReady.find(client) != m_ClientsReady.end());

	// Clients must advance one turn at a time
	ENSURE(turn == m_ClientsReady[client] + 1);
	m_ClientsReady[client] = turn;

	// Check whether this was the final client to become ready
	CheckClientsReady();
}

void CNetServerTurnManager::CheckClientsReady()
{
	// See if all clients (including self) are ready for a new turn
	for (const std::pair<int, u32>& clientReady : m_ClientsReady)
	{
		NETSERVERTURN_LOG("  %d: %d <=? %d\n", clientReady.first, clientReady.second, m_ReadyTurn);
		if (clientReady.second <= m_ReadyTurn)
			return; // wasn't ready for m_ReadyTurn+1
	}

	++m_ReadyTurn;

	NETSERVERTURN_LOG("CheckClientsReady: ready for turn %d\n", m_ReadyTurn);

	// Tell all clients that the next turn is ready
	CEndCommandBatchMessage msg;
	msg.m_TurnLength = m_TurnLength;
	msg.m_Turn = m_ReadyTurn;
	m_NetServer.Broadcast(&msg, { NSS_INGAME });

	ENSURE(m_SavedTurnLengths.size() == m_ReadyTurn);
	m_SavedTurnLengths.push_back(m_TurnLength);
}

void CNetServerTurnManager::NotifyFinishedClientUpdate(int client, const CStrW& playername, u32 turn, const CStr& hash)
{
	// Clients must advance one turn at a time
	ENSURE(turn == m_ClientsSimulated[client] + 1);
	m_ClientsSimulated[client] = turn;

	// Check for OOS only if in sync
	if (m_HasSyncError)
		return;

	m_ClientPlayernames[client] = playername;
	m_ClientStateHashes[turn][client] = hash;

	// Find the newest turn which we know all clients have simulated
	u32 newest = std::numeric_limits<u32>::max();
	for (const std::pair<int, u32>& clientSimulated : m_ClientsSimulated)
		if (clientSimulated.second < newest)
			newest = clientSimulated.second;

	// For every set of state hashes that all clients have simulated, check for OOS
	for (const std::pair<u32, std::map<int, std::string>>& clientStateHash : m_ClientStateHashes)
	{
		if (clientStateHash.first > newest)
			break;

		// Assume the host is correct (maybe we should choose the most common instead to help debugging)
		std::string expected = clientStateHash.second.begin()->second;

		// Find all players that are OOS on that turn
		std::vector<CStrW> OOSPlayerNames;
		for (const std::pair<int, std::string>& hashPair : clientStateHash.second)
		{
			NETSERVERTURN_LOG("sync check %d: %d = %hs\n", it->first, cit->first, Hexify(cit->second).c_str());
			if (hashPair.second != expected)
			{
				// Oh no, out of sync
				m_HasSyncError = true;
				OOSPlayerNames.push_back(m_ClientPlayernames[hashPair.first]);
			}
		}

		// Tell everyone about it
		if (m_HasSyncError)
		{
			CSyncErrorMessage msg;
			msg.m_Turn = clientStateHash.first;
			msg.m_HashExpected = expected;
			for (const CStrW& playername : OOSPlayerNames)
			{
				CSyncErrorMessage::S_m_PlayerNames h;
				h.m_Name = playername;
				msg.m_PlayerNames.push_back(h);
			}
			m_NetServer.Broadcast(&msg, { NSS_INGAME });
			break;
		}
	}

	// Delete the saved hashes for all turns that we've already verified
	m_ClientStateHashes.erase(m_ClientStateHashes.begin(), m_ClientStateHashes.lower_bound(newest+1));
}

void CNetServerTurnManager::InitialiseClient(int client, u32 turn)
{
	NETSERVERTURN_LOG("InitialiseClient(client=%d, turn=%d)\n", client, turn);

	ENSURE(m_ClientsReady.find(client) == m_ClientsReady.end());
	m_ClientsReady[client] = turn + 1;
	m_ClientsSimulated[client] = turn;
}

void CNetServerTurnManager::UninitialiseClient(int client)
{
	NETSERVERTURN_LOG("UninitialiseClient(client=%d)\n", client);

	ENSURE(m_ClientsReady.find(client) != m_ClientsReady.end());
	m_ClientsReady.erase(client);
	m_ClientsSimulated.erase(client);

	// Check whether we're ready for the next turn now that we're not
	// waiting for this client any more
	CheckClientsReady();
}

void CNetServerTurnManager::SetTurnLength(u32 msecs)
{
	m_TurnLength = msecs;
}

u32 CNetServerTurnManager::GetSavedTurnLength(u32 turn)
{
	ENSURE(turn <= m_ReadyTurn);
	return m_SavedTurnLengths.at(turn);
}
