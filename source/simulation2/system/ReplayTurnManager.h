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

#ifndef INCLUDED_REPLAYTURNMANAGER
#define INCLUDED_REPLAYTURNMANAGER

#include "LocalTurnManager.h"

/**
 * Implementation of CLocalTurnManager for replay games.
 */
class CReplayTurnManager : public CLocalTurnManager
{
public:
	CReplayTurnManager(CSimulation2& simulation, IReplayLogger& replay);

	void StoreReplayCommand(u32 turn, int player, const std::string& command);

	void StoreReplayTurnLength(u32 turn, u32 turnLength);

	void StoreReplayHash(u32 turn, const std::string& hash, bool quick);

	void StoreFinalReplayTurn(u32 turn);

private:
	void NotifyFinishedUpdate(u32 turn) override;

	void DoTurn(u32 turn);

	void OnSyncError(u32 turn);

	// Contains the commands of every player on each turn
	std::map<u32, std::vector<std::pair<player_id_t, std::string>>> m_ReplayCommands;

	// Contains the length of every turn
	std::map<u32, u32> m_ReplayTurnLengths;

	// Contains all replay hash values and weather or not the quick hash method was used
	std::map<u32, std::pair<std::string, bool>> m_ReplayHash;
};

#endif // INCLUDED_REPLAYTURNMANAGER
