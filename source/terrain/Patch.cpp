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

	m_Bounds.m_BoxMin.X = m_pVertices[0].m_Position.X;
	m_Bounds.m_BoxMin.Z = m_pVertices[0].m_Position.Z;
	
	m_Bounds.m_BoxMax.X = m_Bounds.m_BoxMin.X + PATCH_SIZE*CELL_SIZE;
	m_Bounds.m_BoxMax.Z = m_Bounds.m_BoxMin.Z + PATCH_SIZE*CELL_SIZE;

	m_Bounds.m_BoxMin.Y = m_Bounds.m_BoxMin.Y = m_pVertices[0].m_Position.Y;

	for (int j=0; j<PATCH_SIZE+1; j++)
	{
		for (int i=0; i<PATCH_SIZE+1; i++)
		{
			int pos = j*MAP_SIZE + i;

			if (m_pVertices[pos].m_Position.Y < m_Bounds.m_BoxMin.Y)
				m_Bounds.m_BoxMin.Y = m_pVertices[pos].m_Position.Y;

			if (m_pVertices[pos].m_Position.Y > m_Bounds.m_BoxMax.Y)
				m_Bounds.m_BoxMax.Y = m_pVertices[pos].m_Position.Y;
		}
	}

	for (j=0; j<16; j++)
	{
		for (int i=0; i<16; i++)
		{
			int pos = (j*MAP_SIZE) + (i);
			m_MiniPatches[j][i].Initialize ( &m_pVertices[pos] );
		}
	}
}

