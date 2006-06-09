///////////////////////////////////////////////////////////////////////////////
//
// Name:		Patch.h
// Author:		Rich Cross
// Contact:		rich@wildfiregames.com
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _PATCH_H
#define _PATCH_H

#include "MiniPatch.h"
#include "RenderableObject.h"

class CTerrain;

///////////////////////////////////////////////////////////////////////////////
// Terrain Constants:
//
// PATCH_SIZE: number of tiles in each patch
const int	PATCH_SIZE = 16;

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

public:
	// minipatches (tiles) making up the patch
	CMiniPatch m_MiniPatches[PATCH_SIZE][PATCH_SIZE];
	// position of patch in parent terrain grid
	u32 m_X,m_Z;
	// parent terrain
	CTerrain* m_Parent;
};


#endif
