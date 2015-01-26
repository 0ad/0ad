/* Copyright (C) 2010 Wildfire Games.
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

#ifndef INCLUDED_ICMPCOMMANDQUEUE
#define INCLUDED_ICMPCOMMANDQUEUE

#include "simulation2/system/Interface.h"

#include "simulation2/helpers/SimulationCommand.h"

/**
 * Command queue, for sending orders to entities.
 * Each command is associated with a player ID (who triggered the command, in some sense)
 * and an arbitrary script value.
 *
 * Commands can be added to the local queue at any time, and will all be executed at the start
 * of the next turn. (This will typically be used by AI scripts.)
 *
 * Alternatively, commands can be sent to the networking system, and they will be executed
 * at the start of some later turn by all players simultaneously. (This will typically be
 * used for user inputs.)
 */
class ICmpCommandQueue : public IComponent
{
public:
	/**
	 * Pushes a new command onto the local queue. @p cmd does not need to be rooted.
	 */
	virtual void PushLocalCommand(player_id_t player, JS::HandleValue cmd) = 0;

	/**
	 * Send a command associated with the current player to the networking system.
	 */
	virtual void PostNetworkCommand(JS::HandleValue cmd) = 0;

	/**
	 * Calls the ProcessCommand(player, cmd) global script function for each command in the
	 * local queue and in @p commands, and empties the local queue.
	 */
	virtual void FlushTurn(const std::vector<SimulationCommand>& commands) = 0;

	DECLARE_INTERFACE_TYPE(CommandQueue)
};

#endif // INCLUDED_ICMPCOMMANDQUEUE
