/* Copyright (C) 2011 Wildfire Games.
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

#ifndef INCLUDED_ICMPAIMANAGER
#define INCLUDED_ICMPAIMANAGER

#include "simulation2/system/Interface.h"

#include "simulation2/helpers/Player.h"

class ICmpAIManager : public IComponent
{
public:
	/**
	 * Add a new AI player into the world, based on the AI script identified
	 * by @p id (corresponding to a subdirectory in simulation/ai/),
	 * to control player @p player.
	 */
	virtual void AddPlayer(const std::wstring& id, player_id_t player, uint8_t difficulty) = 0;
	virtual void SetRNGSeed(uint32_t seed) = 0;
	virtual void TryLoadSharedComponent() = 0;
	virtual void RunGamestateInit() = 0;

	/**
	 * Call this at the end of a turn, to trigger AI computation which will be
	 * ready for the next turn.
	 */
	virtual void StartComputation() = 0;

	/**
	 * Call this at the start of a turn, to push the computed AI commands into
	 * the command queue.
	 */
	virtual void PushCommands() = 0;

	/**
	 * Returns a vector of {"id":"value-for-AddPlayer", "name":"Human readable name"}
	 * objects, based on all the available AI scripts.
	 */
	static JS::Value GetAIs(ScriptInterface& scriptInterface);

	DECLARE_INTERFACE_TYPE(AIManager)
};

#endif // INCLUDED_ICMPAIMANAGER
