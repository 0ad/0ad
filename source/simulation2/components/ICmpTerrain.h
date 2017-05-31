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

#ifndef INCLUDED_ICMPTERRAIN
#define INCLUDED_ICMPTERRAIN

#include "simulation2/system/Interface.h"

#include "simulation2/helpers/Position.h"

#include "maths/FixedVector3D.h"

class CTerrain;
class CVector3D;

class ICmpTerrain : public IComponent
{
public:
	virtual bool IsLoaded() const = 0;

	virtual CFixedVector3D CalcNormal(entity_pos_t x, entity_pos_t z) const = 0;

	virtual CVector3D CalcExactNormal(float x, float z) const = 0;

	virtual entity_pos_t GetGroundLevel(entity_pos_t x, entity_pos_t z) const = 0;

	virtual float GetExactGroundLevel(float x, float z) const = 0;

	/**
	 * Returns number of tiles per side on the terrain.
	 * Return value is always non-zero.
	 */
	virtual u16 GetTilesPerSide() const = 0;

	/**
	 * Returns number of vertices per side on the terrain.
	 * Return value is always non-zero.
	 */
	virtual u16 GetVerticesPerSide() const = 0;

	/**
	 * Returns the map size in metres (world space units).
	 */
	virtual u32 GetMapSize() const = 0;

	virtual CTerrain* GetCTerrain() = 0;

	/**
	 * Call when the underlying CTerrain has been modified behind our backs.
	 * (TODO: eventually we should manage the CTerrain in this class so nobody
	 * can modify it behind our backs).
	 */
	virtual void ReloadTerrain(bool ReloadWater = true) = 0;

	/**
	 * Indicate that terrain tiles within the given region (inclusive lower bound,
	 * exclusive upper bound) have been changed. CMessageTerrainChanged will be
	 * sent to any components that care about terrain changes.
	 */
	virtual void MakeDirty(i32 i0, i32 j0, i32 i1, i32 j1) = 0;

	DECLARE_INTERFACE_TYPE(Terrain)
};

#endif // INCLUDED_ICMPTERRAIN
