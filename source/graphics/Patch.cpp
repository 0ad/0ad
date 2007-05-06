///////////////////////////////////////////////////////////////////////////////
//
// Name:		ModelDef.cpp
// Author:		Rich Cross
// Contact:		rich@wildfiregames.com
// Modified by:	30. April 2007 - Christian Heubach
//
///////////////////////////////////////////////////////////////////////////////

#include "precompiled.h"

#include "Patch.h"
#include "Terrain.h"


///////////////////////////////////////////////////////////////////////////////
// CPatch constructor
CPatch::CPatch() : m_Parent(0)
{
	this->m_bWillBeDrawn = false;
}

///////////////////////////////////////////////////////////////////////////////
// CPatch destructor
CPatch::~CPatch()
{
	
}

///////////////////////////////////////////////////////////////////////////////
// Initialize: setup patch data
void CPatch::Initialize(CTerrain* parent,u32 x,u32 z)
{
	delete m_RenderData;
	m_RenderData=0;

	m_Parent=parent;
	m_X=x;
	m_Z=z;

	// set parent of each patch	
	for (int j=0;j<PATCH_SIZE;j++) {
		for (int i=0;i<PATCH_SIZE;i++) {
			m_MiniPatches[j][i].m_Parent=this;
		}
	}

	InvalidateBounds();

	// get the neightbors
	this->m_Neightbors[CPATCH_NEIGHTBOR_LEFT_TOP] = 
		this->m_Parent->GetPatch(x-1,z-1);
	this->m_Neightbors[CPATCH_NEIGHTBOR_TOP] = 
		this->m_Parent->GetPatch(x,z-1);
	this->m_Neightbors[CPATCH_NEIGHTBOR_RIGHT_TOP] = 
		this->m_Parent->GetPatch(x+1,z-1);
	this->m_Neightbors[CPATCH_NEIGHTBOR_LEFT] = 
		this->m_Parent->GetPatch(x-1,z);
	this->m_Neightbors[CPATCH_NEIGHTBOR_RIGHT] = 
		this->m_Parent->GetPatch(x+1,z);
	this->m_Neightbors[CPATCH_NEIGHTBOR_LEFT_BOTTOM] = 
		this->m_Parent->GetPatch(x-1,z+1);
	this->m_Neightbors[CPATCH_NEIGHTBOR_BOTTOM] = 
		this->m_Parent->GetPatch(x,z+1);
	this->m_Neightbors[CPATCH_NEIGHTBOR_RIGHT_BOTTOM] = 
		this->m_Parent->GetPatch(x+1,z+1);
}

///////////////////////////////////////////////////////////////////////////////
// CalcBounds: calculating the bounds of this patch
void CPatch::CalcBounds()
{
	m_Bounds.SetEmpty();

	for (int j=0;j<PATCH_SIZE+1;j++) {
		for (int i=0;i<PATCH_SIZE+1;i++) {
			CVector3D pos;
			m_Parent->CalcPosition(m_X*PATCH_SIZE+i,m_Z*PATCH_SIZE+j,pos);
			m_Bounds+=pos;
		}
	}
}

