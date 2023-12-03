/* Copyright (C) 2021 Wildfire Games.
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

#ifndef INCLUDED_ICMPUNITMOTIONMANAGER
#define INCLUDED_ICMPUNITMOTIONMANAGER

#include "simulation2/system/Interface.h"

class CCmpUnitMotion;

/**
 * UnitMotionManager - handles motion for CCmpUnitMotion.
 * This allows units to push past each other instead of requiring pathfinder computations,
 * making movement much smoother overall.
 */
class ICmpUnitMotionManager : public IComponent
{
public:
	DECLARE_INTERFACE_TYPE(UnitMotionManager)

private:
	/**
	 * This class makes no sense outside of CCmpUnitMotion. This enforces that tight coupling.
	 */
	friend class CCmpUnitMotion;

	virtual void Register(CCmpUnitMotion* component, entity_id_t ent, bool formationController) = 0;
	virtual void Unregister(entity_id_t ent) = 0;

	/**
	 * True if entities are currently in the "Move" phase.
	 */
	virtual bool ComputingMotion() const = 0;

	/**
	 * @return whether pushing is currently enabled or not.
	 */
	virtual bool IsPushingActivated() const = 0;

};

#endif // INCLUDED_ICMPUNITMOTIONMANAGER
