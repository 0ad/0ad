//***********************************************************
//
// Name:		Patch.h
// Last Update: 23/2/02
// Author:		Poya Manouchehri
//
// Description: CPatch is a smaller portion of the terrain.
//				It handles its own rendering
//
//***********************************************************

#ifndef PATCH_H
#define PATCH_H

#include "Matrix3D.h"
#include "Camera.h"
#include "TerrGlobals.h"
#include "MiniPatch.h"
#include "RenderableObject.h"


class CPatch : public CRenderableObject
{
public:
	CPatch();
	~CPatch();

	//initialize the patch
	void Initialize(CTerrain* parent,u32 x,u32 z);

	// calculate and store bounds of this patch
	void CalcBounds();

	// minipatches (tiles) making up the patch
	CMiniPatch m_MiniPatches[16][16];
	// position of patch in parent terrain grid
	u32 m_X,m_Z;
	// parent terrain
	CTerrain* m_Parent;
};


#endif
