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

#ifndef INCLUDED_ICMPPLAYERMANAGER
#define INCLUDED_ICMPPLAYERMANAGER

#include "simulation2/system/Interface.h"

/**
 * Player manager. This maintains the list of players that exist in the game.
 */
class ICmpPlayerManager : public IComponent
{
public:
	virtual void AddPlayer(entity_id_t ent) = 0;

	virtual int32_t GetNumPlayers() = 0;

	virtual entity_id_t GetPlayerByID(int32_t id) = 0;

	DECLARE_INTERFACE_TYPE(PlayerManager)
};

#endif // INCLUDED_ICMPPLAYERMANAGER
