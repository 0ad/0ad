/**
 * =========================================================================
 * File        : MiniPatch.cpp
 * Project     : 0 A.D.
 * Description : Definition of a single terrain tile
 * =========================================================================
 */

#include "precompiled.h"

#include "Patch.h"
#include "MiniPatch.h"
#include "Terrain.h"

///////////////////////////////////////////////////////////////////////////////
// Constructor
CMiniPatch::CMiniPatch() : Tex1(0), Tex1Priority(0), m_Parent(0)
{
}

///////////////////////////////////////////////////////////////////////////////
// Destructor
CMiniPatch::~CMiniPatch()
{
}


///////////////////////////////////////////////////////////////////////////////
// GetTileIndex: get the index of this tile in the root terrain object; 
// on return, parameters x,y contain index in [0,MapSize)
void CMiniPatch::GetTileIndex(ssize_t& x,ssize_t& z)
{
	const ptrdiff_t tindex = this - &m_Parent->m_MiniPatches[0][0];
	x=(m_Parent->m_X*PATCH_SIZE)+tindex%PATCH_SIZE;
	z=(m_Parent->m_Z*PATCH_SIZE)+tindex/PATCH_SIZE;
}

