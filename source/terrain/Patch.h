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

#include "Bound.h"
#include "TerrGlobals.h"
#include "MiniPatch.h"

class CPatch
{
	public:
		CPatch ();
		~CPatch ();

		//initialize the patch
		void Initialize (STerrainVertex *first_vertex);

//	protected:
		CMiniPatch		m_MiniPatches[16][16];

		CBound	m_Bounds;
		unsigned int	m_LastVisFrame;
		
		STerrainVertex	*m_pVertices;
};


#endif