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


CPatch::CPatch ()
{
	m_pVertices = NULL;
}

CPatch::~CPatch ()
{
}

//Initialize the patch
void CPatch::Initialize (STerrainVertex *first_vertex)
{
	m_pVertices = first_vertex;

	m_Bounds.SetEmpty();

	for (int j=0; j<PATCH_SIZE+1; j++)
	{
		for (int i=0; i<PATCH_SIZE+1; i++)
		{
			m_Bounds+=m_pVertices[j*MAP_SIZE + i].m_Position;
		}
	}

	for (int j=0; j<16; j++)
	{
		for (int i=0; i<16; i++)
		{
			int pos = (j*MAP_SIZE) + (i);
			m_MiniPatches[j][i].Initialize ( &m_pVertices[pos] );
		}
	}
}

