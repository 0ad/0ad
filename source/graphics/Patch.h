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
const int	PATCH_SIZE = 16;

///////////////////////////////////////////////////////////////////////////////
// CPatchNeightbors: neighbor - IDs for CPatch

#define	CPATCH_NEIGHBOR_LEFT_TOP		0
#define	CPATCH_NEIGHBOR_TOP				1
#define	CPATCH_NEIGHBOR_RIGHT_TOP		2
#define CPATCH_NEIGHBOR_LEFT			3
#define CPATCH_NEIGHBOR_RIGHT			4
#define CPATCH_NEIGHBOR_LEFT_BOTTOM		5
#define CPATCH_NEIGHBOR_BOTTOM			6
#define CPATCH_NEIGHBOR_RIGHT_BOTTOM	7

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
	void Initialize(CTerrain* parent,u32 x,u32 z);
	// calculate and store bounds of this patch
	void CalcBounds();

	// is alread in the DrawList
	bool m_bWillBeDrawn;

public:
	// minipatches (tiles) making up the patch
	CMiniPatch m_MiniPatches[PATCH_SIZE][PATCH_SIZE];
	// position of patch in parent terrain grid
	u32 m_X,m_Z;
	// parent terrain
	CTerrain* m_Parent;

	// draw state...
	void setDrawState(bool value) { m_bWillBeDrawn = value; };
	bool getDrawState() { return m_bWillBeDrawn; };
};


#endif


