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

/**
 * Command queue, for sending orders to entities.
 * Each command is associated with a player ID (who triggered the command, in some sense)
 * and an arbitrary script value.
 * Commands can be added to the queue at any time, and will all be executed at the start
 * of the next turn.
 *
 * Typically commands will be sent by the GUI to the networking system, which will eventually
 * add them to this queue; the player ID identifies the client who initiated the action, and
 * the command processing code ought to check the player has permission to perform that command
 * (e.g. make sure they only move their own units). The networking system will add a few turns of
 * latency before the commands reach the queue.
 *
 * Alternatively, commands may be added directly to the queue by AI scripts, to emulate a player,
 * in which case the ID identifies the AI player and the network is not involved.
 */
class ICmpCommandQueue : public IComponent
{
public:
	/**
	 * Pushes a new command onto the queue. @p cmd does not need to be rooted.
	 */
	virtual void PushClientCommand(int player, CScriptVal cmd) = 0;

	/**
	 * Calls the ProcessCommand(player, cmd) global script function for each command in the queue,
	 * in order of adding to the queue, and empties the queue.
	 */
	virtual void ProcessCommands() = 0;

	DECLARE_INTERFACE_TYPE(CommandQueue)
};

#endif // INCLUDED_ICMPCOMMANDQUEUE
