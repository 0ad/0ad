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

#ifndef INCLUDED_ICMPTERRAIN
#define INCLUDED_ICMPTERRAIN

#include "simulation2/system/Interface.h"

#include "simulation2/helpers/Position.h"

#include "maths/FixedVector3D.h"

class ICmpTerrain : public IComponent
{
public:
	virtual CFixedVector3D CalcNormal(entity_pos_t x, entity_pos_t z) = 0;

	virtual entity_pos_t GetGroundLevel(entity_pos_t x, entity_pos_t z) = 0;

	virtual float GetExactGroundLevel(float x, float z) = 0;

	/**
	 * Indicate that the terrain within the given region (inclusive lower bound,
	 * exclusive upper bound) has been changed. CMessageTerrainChanged will be
	 * sent to any components that care about terrain changes.
	 */
	virtual void MakeDirty(ssize_t i0, ssize_t j0, ssize_t i1, ssize_t j1) = 0;

	DECLARE_INTERFACE_TYPE(Terrain)
};

#endif // INCLUDED_ICMPTERRAIN
