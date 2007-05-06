///////////////////////////////////////////////////////////////////////////////
//
// Name:		Patch.h
// Author:		Rich Cross
// Contact:		rich@wildfiregames.com
// Modified by:	30. April 2007 - Christian Heubach
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
// CPatchNeightbors: neightbor - IDs for CPatch

#define	CPATCH_NEIGHTBOR_LEFT_TOP		0
#define	CPATCH_NEIGHTBOR_TOP			1
#define	CPATCH_NEIGHTBOR_RIGHT_TOP		2
#define CPATCH_NEIGHTBOR_LEFT			3
#define CPATCH_NEIGHTBOR_RIGHT			4
#define CPATCH_NEIGHTBOR_LEFT_BOTTOM	5
#define CPATCH_NEIGHTBOR_BOTTOM			6
#define CPATCH_NEIGHTBOR_RIGHT_BOTTOM	7

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

	// neightbors of this patch
	CPatch *m_Neightbors[8];

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
	void resetDrawState() { this->m_bWillBeDrawn = false; }
	void setDrawState()
	{
		for(int i=0;i<8;i++)
		{
			if(m_Neightbors[i])
				m_Neightbors[i]->m_bWillBeDrawn = true;
		}
		m_bWillBeDrawn = true;
	}
	bool getDrawState() { return this->m_bWillBeDrawn; }
};


#endif

