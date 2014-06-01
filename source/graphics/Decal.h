/* Copyright (C) 2013 Wildfire Games.
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

#ifndef INCLUDED_DECAL
#define INCLUDED_DECAL

#include "graphics/Material.h"
#include "graphics/ModelAbstract.h"
#include "graphics/Texture.h"

class CTerrain;

/**
 * Terrain decal definition.
 * Decals are rectangular textures that are projected vertically downwards
 * onto the terrain.
 */
struct SDecal
{
	SDecal(const CMaterial& material, float sizeX, float sizeZ, float angle,
			float offsetX, float offsetZ, bool floating)
		: m_Material(material), m_SizeX(sizeX), m_SizeZ(sizeZ), m_Angle(angle),
		  m_OffsetX(offsetX), m_OffsetZ(offsetZ), m_Floating(floating)
	{
	}

	CMaterial m_Material;
	float m_SizeX;
	float m_SizeZ;
	float m_Angle;
	float m_OffsetX;
	float m_OffsetZ;
	bool m_Floating;
};

class CModelDecal : public CModelAbstract
{
public:
	CModelDecal(CTerrain* terrain, const SDecal& decal)
		: m_Terrain(terrain), m_Decal(decal)
	{
		ENSURE(terrain != NULL);
	}

	/// Dynamic cast
	virtual CModelDecal* ToCModelDecal()
	{
		return this;
	}

	virtual CModelAbstract* Clone() const;

	virtual void SetDirtyRec(int dirtyflags)
	{
		SetDirty(dirtyflags);
	}

	virtual void SetTerrainDirty(ssize_t i0, ssize_t j0, ssize_t i1, ssize_t j1);

	virtual void CalcBounds();
	virtual void ValidatePosition();
	virtual void InvalidatePosition();
	virtual void SetTransform(const CMatrix3D& transform);

	// remove shadow receiving
	void RemoveShadows();

	/**
	 * Compute the terrain vertex indexes that bound the decal's
	 * projection onto the terrain.
	 * The returned indexes are clamped to the terrain size.
	 */
	void CalcVertexExtents(ssize_t& i0, ssize_t& j0, ssize_t& i1, ssize_t& j1);

	CTerrain* m_Terrain;
	SDecal m_Decal;
};

#endif // INCLUDED_DECAL
