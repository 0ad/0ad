/* Copyright (C) 2009 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

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

