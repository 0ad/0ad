///////////////////////////////////////////////////////////////////////////////
//
// Name:		ModelDef.cpp
// Author:		Rich Cross
// Contact:		rich@wildfiregames.com
//
///////////////////////////////////////////////////////////////////////////////

#include "Patch.h"
#include "Terrain.h"


///////////////////////////////////////////////////////////////////////////////
// CPatch constructor
CPatch::CPatch() : m_Parent(0)
{
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
	for (int j=0;j<16;j++) {
		for (int i=0;i<16;i++) {
			m_MiniPatches[j][i].m_Parent=this;
		}
	}

	CalcBounds();
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

