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

#include "precompiled.h"

#include "SimState.h"

#include "ps/Game.h"
#include "simulation2/Simulation2.h"

SimState* SimState::Freeze()
{
	SimState* simState = new SimState();

	if (! g_Game->GetSimulation2()->SerializeState(simState->stream))
	{
		delete simState;
		return NULL;
	}

	return simState;
}

void SimState::Thaw()
{
	g_Game->GetSimulation2()->DeserializeState(stream);
}
