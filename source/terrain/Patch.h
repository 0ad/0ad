//***********************************************************
//
// Name:		Patch.H
// Last Update: 23/2/02
// Author:		Poya Manouchehri
//
// Description: CPatch is a smaller portion of the terrain.
//				It handles its own rendering
//
//***********************************************************

#ifndef PATCH_H
#define PATCH_H

#include "Matrix3D.H"
#include "Camera.H"
#include "TerrGlobals.H"
#include "MiniPatch.H"

class CPatch
{
	public:
		CPatch ();
		~CPatch ();

		//initialize the patch
		void Initialize (STerrainVertex *first_vertex);

//	protected:
		CMiniPatch		m_MiniPatches[16][16];

		SBoundingBox	m_Bounds;
		unsigned int	m_LastVisFrame;
		
		STerrainVertex	*m_pVertices;
};


#endif