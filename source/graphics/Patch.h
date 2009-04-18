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

/// neighbor IDs for CPatch
enum CPatchNeighbors
{
	CPATCH_NEIGHTBOR_LEFT_TOP     = 0,
	CPATCH_NEIGHTBOR_TOP          = 1,
	CPATCH_NEIGHTBOR_RIGHT_TOP    = 2,
	CPATCH_NEIGHTBOR_LEFT         = 3,
	CPATCH_NEIGHTBOR_RIGHT        = 4,
	CPATCH_NEIGHTBOR_LEFT_BOTTOM  = 5,
	CPATCH_NEIGHTBOR_BOTTOM       = 6,
	CPATCH_NEIGHTBOR_RIGHT_BOTTOM = 7
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

	// is already in the DrawList
	bool m_bWillBeDrawn;

public:
	// minipatches (tiles) making up the patch
	CMiniPatch m_MiniPatches[PATCH_SIZE][PATCH_SIZE];
	// position of patch in parent terrain grid
	int m_X,m_Z;
	// parent terrain
	CTerrain* m_Parent;

	// draw state...
	void setDrawState(bool value) { m_bWillBeDrawn = value; };
	bool getDrawState() { return m_bWillBeDrawn; };
};


#endif


