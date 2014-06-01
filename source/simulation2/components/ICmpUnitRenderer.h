/* Copyright (C) 2014 Wildfire Games.
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

#ifndef INCLUDED_ICMPMODELRENDERER
#define INCLUDED_ICMPMODELRENDERER

#include "simulation2/system/Interface.h"

class CUnit;
class CBoundingSphere;
class CVector3D;

class ICmpUnitRenderer : public IComponent
{
public:
	/**
	 * External identifiers for models.
	 * (This is a struct rather than a raw u32 for type-safety.)
	 */
	struct tag_t
	{
		tag_t() : n(0) {}
		explicit tag_t(u32 n) : n(n) {}
		bool valid() { return n != 0; }

		u32 n;
	};

	enum
	{
		ACTOR_ONLY = 1 << 0,
		VISIBLE_IN_ATLAS_ONLY = 1 << 1,
	};

	virtual tag_t AddUnit(CEntityHandle entity, CUnit* unit, const CBoundingSphere& boundsApprox, int flags) = 0;

	virtual void RemoveUnit(tag_t tag) = 0;

	virtual void UpdateUnit(tag_t tag, CUnit* unit, const CBoundingSphere& boundsApprox) = 0;

	virtual void UpdateUnitPos(tag_t tag, bool inWorld, const CVector3D& pos0, const CVector3D& pos1) = 0;

	/**
	 * Returns the frame offset from the last Interpolate message.
	 */
	virtual float GetFrameOffset() = 0;

	/**
	 * Toggle the rendering of debug info.
	 */
	virtual void SetDebugOverlay(bool enabled) = 0;

	DECLARE_INTERFACE_TYPE(UnitRenderer)
};

#endif // INCLUDED_ICMPMODELRENDERER
