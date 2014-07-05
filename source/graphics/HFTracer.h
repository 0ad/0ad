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

/*
 * Determine intersection of rays with a heightfield.
 */

#ifndef INCLUDED_HFTRACER
#define INCLUDED_HFTRACER

class CPatch;
class CVector3D;
class CTerrain;

///////////////////////////////////////////////////////////////////////////////
// CHFTracer: a class for determining ray intersections with a heightfield
class CHFTracer
{
public:
	// constructor; setup data
	CHFTracer(CTerrain *pTerrain);

	// intersect ray with this heightfield; return true if intersection 
	// occurs (and fill in grid coordinates and point of intersection), or false otherwise
	bool RayIntersect(const CVector3D& origin, const CVector3D& dir, int& x, int& z, CVector3D& ipt) const;

	/**
	 * Intersects ray with a single patch.
	 * The ray is a half-infinite line starting at @p origin with direction @p dir
	 * (not required to be a unit vector).. The patch is treated as a collection
	 * of two-sided triangles, corresponding to the terrain tiles.
	 *
	 * If there is an intersection, returns true; and if @p out is not NULL, it
	 * is set to the intersection point. This is guaranteed to be the earliest
	 * tile intersected (starting at @p origin), but not necessarily the earlier
	 * triangle inside that tile.
	 *
	 * This partly duplicates RayIntersect, but it only operates on a single
	 * patch, and it's more precise (it uses the same tile triangulation as the
	 * renderer), and tries to be more numerically robust.
	 */
	static bool PatchRayIntersect(CPatch* patch, const CVector3D& origin, const CVector3D& dir, CVector3D* out);

private:
	// test if ray intersects either of the triangles in the given 
	bool CellIntersect(int cx, int cz, const CVector3D& origin, const CVector3D& dir, float& dist) const;
	
	// The terrain we're operating on
	CTerrain *m_pTerrain;
	// the heightfield were tracing
	const u16* m_Heightfield;
	// size of the heightfield
	size_t m_MapSize;
	// cell size - size of each cell in x and z
	float m_CellSize;
	// vertical scale - size of each cell in y
	float m_HeightScale;
};

#endif
