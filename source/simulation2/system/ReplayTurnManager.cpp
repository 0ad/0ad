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

#include "ReplayTurnManager.h"

#include "gui/GUIManager.h"
#include "ps/Util.h"
#include "simulation2/Simulation2.h"

CReplayTurnManager::CReplayTurnManager(CSimulation2& simulation, IReplayLogger& replay)
	: CLocalTurnManager(simulation, replay)
{
}

void CReplayTurnManager::StoreReplayCommand(u32 turn, int player, const std::string& command)
{
	// Using the pair we make sure that commands per turn will be processed in the correct order
	m_ReplayCommands[turn].emplace_back(player, command);
}

void CReplayTurnManager::StoreReplayHash(u32 turn, const std::string& hash, bool quick)
{
	m_ReplayHash[turn] = std::make_pair(hash, quick);
}

void CReplayTurnManager::StoreReplayTurnLength(u32 turn, u32 turnLength)
{
	m_ReplayTurnLengths[turn] = turnLength;

	// Initialize turn length
	if (turn == 0)
		m_TurnLength = m_ReplayTurnLengths[0];
}

void CReplayTurnManager::StoreFinalReplayTurn(u32 turn)
{
	m_FinalTurn = turn;
}

void CReplayTurnManager::NotifyFinishedUpdate(u32 turn)
{
	if (turn == 1 && m_FinalTurn == 0)
		g_GUI->SendEventToAll("ReplayFinished");

	if (turn > m_FinalTurn)
		return;

	DoTurn(turn);

	// Compare hash if it exists in the replay and if we didn't have an OOS already
	std::map<u32, std::pair<std::string, bool>>::iterator turnHashIt = m_ReplayHash.find(turn);
	if (m_HasSyncError || turnHashIt == m_ReplayHash.end())
		return;

	std::string expectedHash = turnHashIt->second.first;
	bool quickHash = turnHashIt->second.second;

	// Compute hash
	std::string hash;
	ENSURE(m_Simulation2.ComputeStateHash(hash, quickHash));
	hash = Hexify(hash);

	if (hash != expectedHash)
	{
		m_HasSyncError = true;
		LOGERROR("Replay out of sync on turn %d", turn);
		g_GUI->SendEventToAll("ReplayOutOfSync");
	}
}

void CReplayTurnManager::DoTurn(u32 turn)
{
	debug_printf("Executing turn %u of %u\n", turn, m_FinalTurn);

	m_TurnLength = m_ReplayTurnLengths[turn];

	JSContext* cx = m_Simulation2.GetScriptInterface().GetContext();
	JSAutoRequest rq(cx);

	// Simulate commands for that turn
	for (const std::pair<player_id_t, std::string>& p : m_ReplayCommands[turn])
	{
		JS::RootedValue command(cx);
		m_Simulation2.GetScriptInterface().ParseJSON(p.second, &command);
		AddCommand(m_ClientId, p.first, command, m_CurrentTurn + 1);
	}

	if (turn == m_FinalTurn)
		g_GUI->SendEventToAll("ReplayFinished");
}
