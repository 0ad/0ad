/* Copyright (C) 2015 Wildfire Games.
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

#ifndef INCLUDED_SIMULATIONCOMMAND
#define INCLUDED_SIMULATIONCOMMAND

#include "scriptinterface/ScriptInterface.h"
#include "simulation2/helpers/Player.h"

/**
 * Simulation command, typically received over the network in multiplayer games.
 */
struct SimulationCommand
{
	SimulationCommand(player_id_t player, JSContext* cx, JS::HandleValue val)
		: player(player), data(cx, val)
	{
	}

	SimulationCommand(SimulationCommand&& cmd)
		: player(cmd.player), data(cmd.data)
	{
	}

	// std::vector::insert requires the move assignment operator at compilation time,
	// but apparently never uses it (it uses the move constructor).
	SimulationCommand& operator=(SimulationCommand&& other)
	{
		this->player = other.player;
		this->data = other.data;
		return *this;
	}

	player_id_t player;
	JS::PersistentRootedValue data;
};

#endif // INCLUDED_SIMULATIONCOMMAND
