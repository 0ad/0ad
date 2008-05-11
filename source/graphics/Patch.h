/**
 * =========================================================================
 * File        : Patch.h
 * Project     : 0 A.D.
 * Description : A patch of terrain holding NxN MiniPatch tiles
 * =========================================================================
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


