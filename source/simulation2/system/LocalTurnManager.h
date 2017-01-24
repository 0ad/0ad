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

#ifndef INCLUDED_LOCALTURNMANAGER
#define INCLUDED_LOCALTURNMANAGER

#include "TurnManager.h"

/**
 * Implementation of CTurnManager for offline games.
 */
class CLocalTurnManager : public CTurnManager
{
public:
	CLocalTurnManager(CSimulation2& simulation, IReplayLogger& replay);

	void OnSimulationMessage(CSimulationMessage* msg) override;

	void PostCommand(JS::HandleValue data) override;

protected:
	void NotifyFinishedOwnCommands(u32 turn) override;

	virtual void NotifyFinishedUpdate(u32 turn) override;
};

#endif // INCLUDED_LOCALTURNMANAGER
