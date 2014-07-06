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

#include "precompiled.h"

#include "HFTracer.h"

#include "graphics/Patch.h"
#include "graphics/Terrain.h"
#include "maths/BoundingBoxAligned.h"
#include "maths/MathUtil.h"
#include "maths/Vector3D.h"

#include <cfloat>

// To cope well with points that are slightly off the edge of the map,
// we act as if there's an N-tile margin around the edges of the heightfield.
// (N shouldn't be too huge else it'll hurt performance a little when
// RayIntersect loops through it all.)
// CTerrain::CalcPosition implements clamp-to-edge behaviour so the tracer
// will have that behaviour.
static const int MARGIN_SIZE = 64;

///////////////////////////////////////////////////////////////////////////////
// CHFTracer constructor
CHFTracer::CHFTracer(CTerrain *pTerrain):
	m_pTerrain(pTerrain),
	m_Heightfield(m_pTerrain->GetHeightMap()),
	m_MapSize(m_pTerrain->GetVerticesPerSide()),
	m_CellSize((float)TERRAIN_TILE_SIZE),
	m_HeightScale(HEIGHT_SCALE)
{
}


///////////////////////////////////////////////////////////////////////////////
// RayTriIntersect: intersect a ray with triangle defined by vertices
// v0,v1,v2; return true if ray hits triangle at distance less than dist,
// or false otherwise
static bool RayTriIntersect(const CVector3D& v0, const CVector3D& v1, const CVector3D& v2,
								const CVector3D& origin, const CVector3D& dir, float& dist)
{
	const float EPSILON=0.00001f;

	// calculate edge vectors
	CVector3D edge0=v1-v0;
	CVector3D edge1=v2-v0;

    // begin calculating determinant - also used to calculate U parameter
    CVector3D pvec=dir.Cross(edge1);

    // if determinant is near zero, ray lies in plane of triangle
    float det = edge0.Dot(pvec);
    if (fabs(det)<EPSILON)
        return false;

    float inv_det = 1.0f/det;

    // calculate vector from vert0 to ray origin
    CVector3D tvec=origin-v0;

    // calculate U parameter, test bounds
    float u=tvec.Dot(pvec)*inv_det;
    if (u<-0.01f || u>1.01f)
        return false;

    // prepare to test V parameter
    CVector3D qvec=tvec.Cross(edge0);

    // calculate V parameter and test bounds
    float v=dir.Dot(qvec)*inv_det;
    if (v<0.0f || u+v>1.0f)
        return false;

    // calculate distance to intersection point from ray origin
    float d=edge1.Dot(qvec)*inv_det;
    if (d>=0 && d<dist) {
        dist=d;
        return true;
    }

    return false;
}

///////////////////////////////////////////////////////////////////////////////
// CellIntersect: test if ray intersects either of the triangles in the given
// cell - return hit result, and distance to hit, if hit occurred
bool CHFTracer::CellIntersect(int cx, int cz, const CVector3D& origin, const CVector3D& dir, float& dist) const
{
	bool res=false;

	// get vertices for this cell
	CVector3D vpos[4];
	m_pTerrain->CalcPosition(cx,cz,vpos[0]);
	m_pTerrain->CalcPosition(cx+1,cz,vpos[1]);
	m_pTerrain->CalcPosition(cx+1,cz+1,vpos[2]);
	m_pTerrain->CalcPosition(cx,cz+1,vpos[3]);

	dist=1.0e30f;
	if (RayTriIntersect(vpos[0],vpos[1],vpos[2],origin,dir,dist)) {
		res=true;
	}

	if (RayTriIntersect(vpos[0],vpos[2],vpos[3],origin,dir,dist)) {
		res=true;
	}

	return res;
}

///////////////////////////////////////////////////////////////////////////////
// RayIntersect: intersect ray with this heightfield; return true if
// intersection occurs (and fill in grid coordinates of intersection), or false
// otherwise
bool CHFTracer::RayIntersect(const CVector3D& origin, const CVector3D& dir, int& x, int& z, CVector3D& ipt) const
{
	// If the map is empty (which should never happen),
	// return early before we crash when reading zero-sized heightmaps
	if (!m_MapSize)
	{
		debug_warn(L"CHFTracer::RayIntersect called with zero-size map");
		return false;
	}

	// intersect first against bounding box
	CBoundingBoxAligned bound;
	bound[0] = CVector3D(-MARGIN_SIZE * m_CellSize, 0, -MARGIN_SIZE * m_CellSize);
	bound[1] = CVector3D((m_MapSize + MARGIN_SIZE) * m_CellSize, 65535 * m_HeightScale, (m_MapSize + MARGIN_SIZE) * m_CellSize);

	float tmin,tmax;
	if (!bound.RayIntersect(origin,dir,tmin,tmax)) {
		// ray missed world bounds; no intersection
		return false;
	}

	// project origin onto grid, if necessary, to get starting point for traversal
	CVector3D traversalPt;
	if (tmin>0) {
		traversalPt=origin+dir*tmin;
	} else {
		traversalPt=origin;
	}

	// setup traversal variables
	int sx=dir.X<0 ? -1 : 1;
	int sz=dir.Z<0 ? -1 : 1;

	float invCellSize=1.0f/float(m_CellSize);

	float fcx=traversalPt.X*invCellSize;
	int cx=(int)floor(fcx);

	float fcz=traversalPt.Z*invCellSize;
	int cz=(int)floor(fcz);

	float invdx = 1.0e20f;
	float invdz = 1.0e20f;

	if (fabs(dir.X) > 1.0e-20)
		invdx = float(1.0/fabs(dir.X));
	if (fabs(dir.Z) > 1.0e-20)
		invdz = float(1.0/fabs(dir.Z));

	do {
		// test current cell
		if (cx >= -MARGIN_SIZE && cx < int(m_MapSize + MARGIN_SIZE - 1) && cz >= -MARGIN_SIZE && cz < int(m_MapSize + MARGIN_SIZE - 1))
		{
			float dist;

			if (CellIntersect(cx,cz,origin,dir,dist)) {
				x=cx;
				z=cz;
				ipt=origin+dir*dist;
				return true;
			}
		}
		else
		{
			// Degenerate case: y close to zero
			// catch travelling off the map
			if ((cx < -MARGIN_SIZE) && (sx < 0))
				return false;
			if ((cx >= (int)(m_MapSize + MARGIN_SIZE - 1)) && (sx > 0))
				return false;
			if ((cz < -MARGIN_SIZE) && (sz < 0))
				return false;
			if ((cz >= (int)(m_MapSize + MARGIN_SIZE - 1)) && (sz > 0))
				return false;
		}

		// get coords of current cell
		fcx=traversalPt.X*invCellSize;
		fcz=traversalPt.Z*invCellSize;

		// get distance to next cell in x,z
		float dx=(sx==-1) ? fcx-float(cx) : 1-(fcx-float(cx));
		dx*=invdx;
		float dz=(sz==-1) ? fcz-float(cz) : 1-(fcz-float(cz));
		dz*=invdz;

		// advance ..
		float dist;
		if (dx<dz) {
			cx+=sx;
			dist=dx;
		} else {
			cz+=sz;
			dist=dz;
		}

		traversalPt+=dir*dist;
	} while (traversalPt.Y>=0);

	// fell off end of heightmap with no intersection; return a miss
	return false;
}

static bool TestTile(u16* heightmap, int stride, int i, int j, const CVector3D& pos, const CVector3D& dir, CVector3D& isct)
{
	u16 y00 = heightmap[i + j*stride];
	u16 y10 = heightmap[i+1 + j*stride];
	u16 y01 = heightmap[i + (j+1)*stride];
	u16 y11 = heightmap[i+1 + (j+1)*stride];

	CVector3D p00(    i * TERRAIN_TILE_SIZE, y00 * HEIGHT_SCALE,     j * TERRAIN_TILE_SIZE);
	CVector3D p10((i+1) * TERRAIN_TILE_SIZE, y10 * HEIGHT_SCALE,     j * TERRAIN_TILE_SIZE);
	CVector3D p01(    i * TERRAIN_TILE_SIZE, y01 * HEIGHT_SCALE, (j+1) * TERRAIN_TILE_SIZE);
	CVector3D p11((i+1) * TERRAIN_TILE_SIZE, y11 * HEIGHT_SCALE, (j+1) * TERRAIN_TILE_SIZE);

	int mid1 = y00+y11;
	int mid2 = y01+y10;
	int triDir = (mid1 < mid2);

	float dist = FLT_MAX;

	if (triDir)
	{
		if (RayTriIntersect(p00, p10, p01, pos, dir, dist) || // lower-left triangle
		    RayTriIntersect(p11, p01, p10, pos, dir, dist))   // upper-right triangle
		{
			isct = pos + dir * dist;
			return true;
		}
	}
	else
	{
		if (RayTriIntersect(p00, p11, p01, pos, dir, dist) || // upper-left triangle
		    RayTriIntersect(p00, p10, p11, pos, dir, dist))   // lower-right triangle
		{
			isct = pos + dir * dist;
			return true;
		}
	}
	return false;
}

bool CHFTracer::PatchRayIntersect(CPatch* patch, const CVector3D& origin, const CVector3D& dir, CVector3D* out)
{
	// (TODO: This largely duplicates RayIntersect - some refactoring might be
	// nice in the future.)

	// General approach:
	// Given the ray defined by origin + dir * t, we increase t until it
	// enters the patch's bounding box. The x,z coordinates identify which
	// tile it is currently above/below. Do an intersection test vs the tile's
	// two triangles. If it doesn't hit, do a 2D line rasterisation to find
	// the next tiles the ray will pass through, and test each of them.

	// Start by jumping to the point where the ray enters the bounding box
	CBoundingBoxAligned bound = patch->GetWorldBounds();
	float tmin, tmax;
	if (!bound.RayIntersect(origin, dir, tmin, tmax))
	{
		// Ray missed patch; no intersection
		return false;
	}

	int heightmapStride = patch->m_Parent->GetVerticesPerSide();

	// Get heightmap, offset to start at this patch
	u16* heightmap = patch->m_Parent->GetHeightMap() +
			patch->m_X * PATCH_SIZE +
			patch->m_Z * PATCH_SIZE * heightmapStride;

	// Get patch-space position of ray origin and bbox entry point
	CVector3D patchPos(
			patch->m_X * PATCH_SIZE * TERRAIN_TILE_SIZE,
			0.0f,
			patch->m_Z * PATCH_SIZE * TERRAIN_TILE_SIZE);
	CVector3D originPatch = origin - patchPos;
	CVector3D entryPatch = originPatch + dir * tmin;

	// We want to do a simple 2D line rasterisation (with the 3D ray projected
	// down onto the Y plane). That will tell us which cells are intersected
	// in 2D dimensions, then we can do a more precise 3D intersection test.
	//
	// WLOG, assume the ray has direction dir.x > 0, dir.z > 0, and starts in
	// cell (i,j). The next cell intersecting the line must be either (i+1,j)
	// or (i,j+1). To tell which, just check whether the point (i+1,j+1) is
	// above or below the ray. Advance into that cell and repeat.
	//
	// (If the ray passes precisely through (i+1,j+1), we can pick either.
	// If the ray is parallel to Y, only the first cell matters, then we can
	// carry on rasterising in any direction (a bit of a waste of time but
	// should be extremely rare, and it's safe and simple).)

	// Work out which tile we're starting in
	int i = clamp((int)(entryPatch.X / TERRAIN_TILE_SIZE), 0, (int)PATCH_SIZE-1);
	int j = clamp((int)(entryPatch.Z / TERRAIN_TILE_SIZE), 0, (int)PATCH_SIZE-1);

	// Work out which direction the ray is going in
	int di = (dir.X >= 0 ? 1 : 0);
	int dj = (dir.Z >= 0 ? 1 : 0);

	do
	{
		CVector3D isct;
		if (TestTile(heightmap, heightmapStride, i, j, originPatch, dir, isct))
		{
			if (out)
				*out = isct + patchPos;
			return true;
		}

		// Get the vertex between the two possible next cells
		float nx = (i + di) * (int)TERRAIN_TILE_SIZE;
		float nz = (j + dj) * (int)TERRAIN_TILE_SIZE;

		// Test which side of the ray the vertex is on, and advance into the
		// appropriate cell, using a test that works for all 4 combinations
		// of di,dj
		float dot = dir.Z * (nx - originPatch.X) - dir.X * (nz - originPatch.Z);
		if ((di == dj) == (dot > 0.0f))
			j += dj*2-1;
		else
			i += di*2-1;
	}
	while (i >= 0 && j >= 0 && i < PATCH_SIZE && j < PATCH_SIZE);

	// Ran off the edge of the patch, so no intersection
	return false;
}
