/* Copyright (C) 2009 Wildfire Games.
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
 * A patch of terrain holding NxN MiniPatch tiles
 */

#ifndef INCLUDED_PATCH
#define INCLUDED_PATCH

#include "MiniPatch.h"
#include "RenderableObject.h"

class CTerrain;

///////////////////////////////////////////////////////////////////////////////
// Terrain Constants:
//
// PATCH_SIZE: number of tiles in each patch
const ssize_t PATCH_SIZE = 16;

// Flags for whether the patch should be drawn with a solid plane
// on each side
enum CPatchSideFlags
{
	CPATCH_SIDE_NEGX = (1 << 0),
	CPATCH_SIDE_POSX = (1 << 1),
	CPATCH_SIDE_NEGZ = (1 << 2),
	CPATCH_SIDE_POSZ = (1 << 3),
};

///////////////////////////////////////////////////////////////////////////////
// CPatch: a single terrain patch, PATCH_SIZE tiles square
class CPatch : public CRenderableObject
{
public:
	// constructor
	CPatch();
	// destructor
	~CPatch();

	// initialize the patch
	void Initialize(CTerrain* parent,ssize_t x,ssize_t z);
	// calculate and store bounds of this patch
	void CalcBounds();

public:
	// minipatches (tiles) making up the patch
	CMiniPatch m_MiniPatches[PATCH_SIZE][PATCH_SIZE];
	// position of patch in parent terrain grid
	int m_X,m_Z;
	// parent terrain
	CTerrain* m_Parent;

	int GetSideFlags();
};


#endif


