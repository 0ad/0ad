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

#ifndef INCLUDED_ICMPVISIBILITY
#define INCLUDED_ICMPVISIBILITY

#include "simulation2/system/Interface.h"

#include "simulation2/components/ICmpRangeManager.h"

/**
 * The Visibility component is a scripted component that allows any part of the simulation to
 * influence the visibility of an entity.
 *
 * This component:
 * - Holds the template values RetainInFog and AlwaysVisible, used by the range manager to compute
 * the visibility of the entity;
 * - Can supersede the range manager if it is "activated". This is to avoid useless calls to the scripts.
 */

class ICmpVisibility : public IComponent
{
public:
	/**
	 * This function is a fallback for some entities whose visibility status
	 * cannot be cached by the range manager (especially local entities like previews).
	 * Calling the scripts is expensive, so only call it if really needed.
	 */
	virtual bool IsActivated() = 0;

	virtual ICmpRangeManager::ELosVisibility GetVisibility(player_id_t player, bool isVisible, bool isExplored) = 0;

	virtual bool GetRetainInFog() = 0;

	virtual bool GetAlwaysVisible() = 0;

	DECLARE_INTERFACE_TYPE(Visibility)
};

#endif // INCLUDED_ICMPVISIBILITY
