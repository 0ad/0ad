///////////////////////////////////////////////////////////////////////////////
//
// Name:		Patch.h
// Author:		Rich Cross
// Contact:		rich@wildfiregames.com
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _PATCH_H
#define _PATCH_H

///////////////////////////////////////////////////////////////////////////////
// Terrain Constants:
//
// PATCH_SIZE: number of tiles in each patch
const int	PATCH_SIZE = 16;
// CELL_SIZE: size of each tile in x and z
const int	CELL_SIZE = 4;	
// HEIGHT_SCALE: vertical scale of terrain - terrain has a coordinate range of 
// 0 to 65536*HEIGHT_SCALE
const float HEIGHT_SCALE = 0.35f/256.0f;


#include "MiniPatch.h"
#include "RenderableObject.h"

class CTerrain;

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
