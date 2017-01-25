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

#include "LocalTurnManager.h"

CLocalTurnManager::CLocalTurnManager(CSimulation2& simulation, IReplayLogger& replay)
	: CTurnManager(simulation, DEFAULT_TURN_LENGTH_SP, 0, replay)
{
}

void CLocalTurnManager::PostCommand(JS::HandleValue data)
{
	// Add directly to the next turn, ignoring COMMAND_DELAY,
	// because we don't need to compensate for network latency
	AddCommand(m_ClientId, m_PlayerId, data, m_CurrentTurn + 1);
}

void CLocalTurnManager::NotifyFinishedOwnCommands(u32 turn)
{
	FinishedAllCommands(turn, m_TurnLength);
}

void CLocalTurnManager::NotifyFinishedUpdate(u32 UNUSED(turn))
{
#if 0 // this hurts performance and is only useful for verifying log replays
	std::string hash;
	{
		PROFILE3("state hash check");
		ENSURE(m_Simulation2.ComputeStateHash(hash));
	}
	m_Replay.Hash(hash);
#endif
}

void CLocalTurnManager::OnSimulationMessage(CSimulationMessage* UNUSED(msg))
{
	debug_warn(L"This should never be called");
}
