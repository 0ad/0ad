//***********************************************************
//
// Name:		Patch.Cpp
// Last Update: 23/2/02
// Author:		Poya Manouchehri
//
// Description: CPatch is a smaller portion of the terrain.
//				It handles the ROAM implementation and its
//				own rendering.
//
//***********************************************************

#include "Patch.h"
#include "Terrain.h"


CPatch::CPatch()
{
	m_Parent = NULL;
}

CPatch::~CPatch()
{
}

void CPatch::Initialize(CTerrain* parent,u32 x,u32 z)
{
	delete m_RenderData;
	m_RenderData;

	m_Parent=parent;
	m_X=x;
	m_Z=z;

	u32 mapSize=m_Parent->GetVerticesPerSide();

	for (int j=0; j<16; j++) {
		for (int i=0; i<16; i++) {
			m_MiniPatches[j][i].m_Parent=this;
		}
	}

	CalcBounds();
}

void CPatch::CalcBounds()
{
	u32 mapSize=m_Parent->GetVerticesPerSide();

	m_Bounds.SetEmpty();

	for (int j=0; j<PATCH_SIZE+1; j++)
	{
		for (int i=0; i<PATCH_SIZE+1; i++)
		{
			CVector3D pos;
			m_Parent->CalcPosition(m_X*PATCH_SIZE+i,m_Z*PATCH_SIZE+j,pos);
			m_Bounds+=pos;
		}
	}
}

