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

#ifndef INCLUDED_NETCLIENTTURNMANAGER
#define INCLUDED_NETCLIENTTURNMANAGER

#include "simulation2/system/TurnManager.h"
#include "NetMessage.h"

class CNetClient;

/**
 * Implementation of CTurnManager for network clients.
 */
class CNetClientTurnManager : public CTurnManager
{
	NONCOPYABLE(CNetClientTurnManager);
public:
	CNetClientTurnManager(CSimulation2& simulation, CNetClient& client, int clientId, IReplayLogger& replay);

	void OnSimulationMessage(CSimulationMessage* msg) override;

	void PostCommand(JS::HandleValue data) override;

	/**
	 * Notify the server that all commands are sent to prepare the connection for termination.
	 */
	void OnDestroyConnection();

	void OnSyncError(u32 turn, const CStr& expectedHash, const std::vector<CSyncErrorMessage::S_m_PlayerNames>& playerNames);

private:
	void NotifyFinishedOwnCommands(u32 turn) override;

	void NotifyFinishedUpdate(u32 turn) override;

	CNetClient& m_NetClient;
};

#endif // INCLUDED_NETCLIENTTURNMANAGER
