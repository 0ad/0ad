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

#ifndef INCLUDED_ICMPOWNERSHIP
#define INCLUDED_ICMPOWNERSHIP

#include "simulation2/system/Interface.h"
#include "simulation2/helpers/Player.h"

/**
 * Player ownership.
 * Owner values are either a player ID (if >= 0), or unassigned (INVALID_PLAYER).
 * Sends message OwnershipChanged after it changes.
 */
class ICmpOwnership : public IComponent
{
public:
	virtual player_id_t GetOwner() const = 0;

	virtual void SetOwner(player_id_t playerID) = 0;

	// Operates identically to SetOwner() but does not send a message.
	virtual void SetOwnerQuiet(player_id_t playerID) = 0;

	DECLARE_INTERFACE_TYPE(Ownership)
};

#endif // INCLUDED_ICMPOWNERSHIP
