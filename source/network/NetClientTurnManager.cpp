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

#include "NetClientTurnManager.h"
#include "NetClient.h"

#include "gui/GUIManager.h"
#include "ps/CLogger.h"
#include "ps/Pyrogenesis.h"
#include "ps/Replay.h"
#include "ps/Util.h"
#include "simulation2/Simulation2.h"

#if 0
#define NETCLIENTTURN_LOG(...) debug_printf(__VA_ARGS__)
#else
#define NETCLIENTTURN_LOG(...)
#endif

CNetClientTurnManager::CNetClientTurnManager(CSimulation2& simulation, CNetClient& client, int clientId, IReplayLogger& replay)
	: CTurnManager(simulation, DEFAULT_TURN_LENGTH_MP, clientId, replay), m_NetClient(client)
{
}

void CNetClientTurnManager::PostCommand(JS::HandleValue data)
{
	NETCLIENTTURN_LOG("PostCommand()\n");

	// Transmit command to server
	CSimulationMessage msg(m_Simulation2.GetScriptInterface(), m_ClientId, m_PlayerId, m_CurrentTurn + COMMAND_DELAY, data);
	m_NetClient.SendMessage(&msg);

	// Add to our local queue
	//AddCommand(m_ClientId, m_PlayerId, data, m_CurrentTurn + COMMAND_DELAY);
	// TODO: we should do this when the server stops sending our commands back to us
}

void CNetClientTurnManager::NotifyFinishedOwnCommands(u32 turn)
{
	NETCLIENTTURN_LOG("NotifyFinishedOwnCommands(%d)\n", turn);

	CEndCommandBatchMessage msg;

	msg.m_Turn = turn;

	// The turn-length field of the CEndCommandBatchMessage is currently only relevant
	// when sending it from the server to the clients.
	// It could be used to verify that the client simulated the correct turn length.
	msg.m_TurnLength = 0;

	m_NetClient.SendMessage(&msg);
}

void CNetClientTurnManager::NotifyFinishedUpdate(u32 turn)
{
	bool quick = !TurnNeedsFullHash(turn);
	std::string hash;
	{
		PROFILE3("state hash check");
		ENSURE(m_Simulation2.ComputeStateHash(hash, quick));
	}

	NETCLIENTTURN_LOG("NotifyFinishedUpdate(%d, %hs)\n", turn, Hexify(hash).c_str());

	m_Replay.Hash(hash, quick);

	// Don't send the hash if OOS
	if (m_HasSyncError)
		return;

	// Send message to the server
	CSyncCheckMessage msg;
	msg.m_Turn = turn;
	msg.m_Hash = hash;
	m_NetClient.SendMessage(&msg);
}

void CNetClientTurnManager::OnDestroyConnection()
{
	NotifyFinishedOwnCommands(m_CurrentTurn + COMMAND_DELAY);
}

void CNetClientTurnManager::OnSimulationMessage(CSimulationMessage* msg)
{
	// Command received from the server - store it for later execution
	AddCommand(msg->m_Client, msg->m_Player, msg->m_Data, msg->m_Turn);
}

void CNetClientTurnManager::OnSyncError(u32 turn, const CStr& expectedHash, const std::vector<CSyncErrorMessage::S_m_PlayerNames>& playerNames)
{
	NETCLIENTTURN_LOG("OnSyncError(%d, %hs)\n", turn, Hexify(expectedHash).c_str());

	// Only complain the first time
	if (m_HasSyncError)
		return;

	m_HasSyncError = true;

	std::string hash;
	ENSURE(m_Simulation2.ComputeStateHash(hash, !TurnNeedsFullHash(turn)));

	OsPath path = psLogDir() / "oos_dump.txt";
	std::ofstream file (OsString(path).c_str(), std::ofstream::out | std::ofstream::trunc);
	m_Simulation2.DumpDebugState(file);
	file.close();

	hash = Hexify(hash);

	std::stringstream msg;
	msg << "Out of sync on turn " << turn;

	for (size_t i = 0; i < playerNames.size(); ++i)
		msg << (i == 0 ? "\nPlayers: " : ", ") << utf8_from_wstring(playerNames[i].m_Name);

	msg << "\n\n" << "Your game state is " << (expectedHash == hash ? "identical to" : "different from") << " the hosts game state.";

	msg << "\n\n" << "Dumping current state to " << CStr(path.string8()).EscapeToPrintableASCII();

	LOGERROR("%s", msg.str());

	if (g_GUI)
		g_GUI->DisplayMessageBox(600, 350, L"Sync error", wstring_from_utf8(msg.str()));
}
