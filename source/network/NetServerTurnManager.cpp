/* Copyright (C) 2021 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "precompiled.h"

#include "NetMessage.h"
#include "NetServerTurnManager.h"
#include "NetServer.h"
#include "NetSession.h"

#include "lib/utf8.h"
#include "ps/CLogger.h"
#include "ps/ConfigDB.h"
#include "simulation2/system/TurnManager.h"

#if 0
#include "ps/Util.h"
#define NETSERVERTURN_LOG(...) debug_printf(__VA_ARGS__)
#else
#define NETSERVERTURN_LOG(...)
#endif

CNetServerTurnManager::CNetServerTurnManager(CNetServerWorker& server)
	: m_NetServer(server), m_ReadyTurn(COMMAND_DELAY_MP - 1), m_TurnLength(DEFAULT_TURN_LENGTH)
{
	// Turn 0 is not actually executed, store a dummy value.
	m_SavedTurnLengths.push_back(0);
	// Turns [1..COMMAND_DELAY - 1] are special: all clients run them without waiting on a server command batch.
	// Because of this, they are always run with the default MP turn length.
	for (u32 i = 1; i < COMMAND_DELAY_MP; ++i)
		m_SavedTurnLengths.push_back(m_TurnLength);
}

void CNetServerTurnManager::NotifyFinishedClientCommands(CNetServerSession& session, u32 turn)
{
	int client = session.GetHostID();

	NETSERVERTURN_LOG("NotifyFinishedClientCommands(client=%d, turn=%d)\n", client, turn);

	// Must be a client we've already heard of
	ENSURE(m_ClientsData.find(client) != m_ClientsData.end());

	// Clients must advance one turn at a time
	if (turn != m_ClientsData[client].readyTurn + 1)
	{
		LOGERROR("NotifyFinishedClientCommands: Client %d (%s) is ready for turn %d, but expected %d",
			client,
			utf8_from_wstring(session.GetUserName()).c_str(),
			turn,
			m_ClientsData[client].readyTurn + 1);

		session.Disconnect(NDR_INCORRECT_READY_TURN_COMMANDS);
	}

	m_ClientsData[client].readyTurn = turn;

	// Check whether this was the final client to become ready
	CheckClientsReady();
}

void CNetServerTurnManager::CheckClientsReady()
{
	int max_observer_lag = -1;
	CFG_GET_VAL("network.observermaxlag", max_observer_lag);
	// Clamp to 0-10000 turns, below/above that is no limit.
	max_observer_lag = max_observer_lag < 0 ? -1 : max_observer_lag > 10000 ? -1 : max_observer_lag;

	// See if all clients (including self) are ready for a new turn
	for (const std::pair<const int, Client>& clientData : m_ClientsData)
	{
		// Observers are allowed to lag more than regular clients.
		if (clientData.second.isObserver && (max_observer_lag == -1 || clientData.second.readyTurn > m_ReadyTurn - max_observer_lag))
			continue;
		NETSERVERTURN_LOG("  %d: %d <=? %d\n", clientReady.first, clientReady.second, m_ReadyTurn);
		if (clientData.second.readyTurn <= m_ReadyTurn)
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

void CNetServerTurnManager::NotifyFinishedClientUpdate(CNetServerSession& session, u32 turn, const CStr& hash)
{

	int client = session.GetHostID();
	const CStrW& playername = session.GetUserName();

	// Clients must advance one turn at a time
	if (turn != m_ClientsData[client].simulatedTurn + 1)
	{
		LOGERROR("NotifyFinishedClientUpdate: Client %d (%s) is ready for turn %d, but expected %d",
			client,
			utf8_from_wstring(playername).c_str(),
			turn,
			m_ClientsData[client].simulatedTurn + 1);

		session.Disconnect(NDR_INCORRECT_READY_TURN_SIMULATED);
	}

	m_ClientsData[client].simulatedTurn = turn;

	// Check for OOS only if in sync
	if (m_HasSyncError)
		return;

	m_ClientsData[client].playerName = playername;
	m_ClientStateHashes[turn][client] = hash;

	// Find the newest turn which we know all clients have simulated
	u32 newest = std::numeric_limits<u32>::max();
	for (const std::pair<const int, Client>& clientData : m_ClientsData)
		if (clientData.second.simulatedTurn < newest)
			newest = clientData.second.simulatedTurn;

	// For every set of state hashes that all clients have simulated, check for OOS
	for (const std::pair<const u32, std::map<int, std::string>>& clientStateHash : m_ClientStateHashes)
	{
		if (clientStateHash.first > newest)
			break;

		// Assume the host is correct (maybe we should choose the most common instead to help debugging)
		std::string expected = clientStateHash.second.begin()->second;

		// Find all players that are OOS on that turn
		std::vector<CStrW> OOSPlayerNames;
		for (const std::pair<const int, std::string>& hashPair : clientStateHash.second)
		{
			NETSERVERTURN_LOG("sync check %d: %d = %hs\n", clientStateHash.first, hashPair.first, Hexify(hashPair.second).c_str());
			if (hashPair.second != expected)
			{
				// Oh no, out of sync
				m_HasSyncError = true;
				m_ClientsData[hashPair.first].isOOS = true;
				OOSPlayerNames.push_back(m_ClientsData[hashPair.first].playerName);
			}
		}

		// Tell everyone about it
		if (m_HasSyncError)
		{
			CSyncErrorMessage msg;
			msg.m_Turn = clientStateHash.first;
			msg.m_HashExpected = expected;
			for (const CStrW& oosPlayername : OOSPlayerNames)
			{
				CSyncErrorMessage::S_m_PlayerNames h;
				h.m_Name = oosPlayername;
				msg.m_PlayerNames.push_back(h);
			}
			m_NetServer.Broadcast(&msg, { NSS_INGAME });
			break;
		}
	}

	// Delete the saved hashes for all turns that we've already verified
	m_ClientStateHashes.erase(m_ClientStateHashes.begin(), m_ClientStateHashes.lower_bound(newest+1));
}

void CNetServerTurnManager::InitialiseClient(int client, u32 turn, bool observer)
{
	NETSERVERTURN_LOG("InitialiseClient(client=%d, turn=%d)\n", client, turn);

	ENSURE(m_ClientsData.find(client) == m_ClientsData.end());
	Client& data = m_ClientsData[client];
	data.readyTurn = turn + COMMAND_DELAY_MP - 1;
	data.simulatedTurn = turn;
	data.isObserver = observer;
}

void CNetServerTurnManager::UninitialiseClient(int client)
{
	NETSERVERTURN_LOG("UninitialiseClient(client=%d)\n", client);

	ENSURE(m_ClientsData.find(client) != m_ClientsData.end());
	bool checkOOS = m_ClientsData[client].isOOS;
	m_ClientsData.erase(client);

	// Check whether we're ready for the next turn now that we're not
	// waiting for this client any more
	CheckClientsReady();

	// Check whether we're still OOS.
	if (checkOOS)
	{
		for (const std::pair<const int, Client>& clientData : m_ClientsData)
			if (clientData.second.isOOS)
				return;
		m_HasSyncError = false;
	}
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
