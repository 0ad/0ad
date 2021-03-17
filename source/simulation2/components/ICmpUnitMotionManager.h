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

class ICmpPosition;
class ICmpUnitMotion;

class ICmpUnitMotionManager : public IComponent
{
public:
	// Persisted state for each unit.
	struct MotionState
	{
		// Component references - these must be kept alive for the duration of motion.
		CmpPtr<ICmpPosition> cmpPosition;
		CmpPtr<ICmpUnitMotion> cmpUnitMotion;

		// Position before units start moving
		CFixedVector2D initialPos;
		// Transient position during the movement.
		CFixedVector2D pos;

		fixed initialAngle;
		fixed angle;

		// If true, the entity needs to be handled during movement.
		bool needUpdate;

		// 'Leak' from UnitMotion.
		bool wentStraight;
		bool wasObstructed;
	};

	virtual void Register(entity_id_t ent, bool formationController) = 0;
	virtual void Unregister(entity_id_t ent) = 0;

	/**
	 * True if entities are currently in the "Move" phase.
	 */
	virtual bool ComputingMotion() const = 0;

	DECLARE_INTERFACE_TYPE(UnitMotionManager)
};

#endif // INCLUDED_ICMPUNITMOTIONMANAGER
