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
 * File        : Patch.cpp
 * Project     : 0 A.D.
 * Description : A patch of terrain holding NxN MiniPatch tiles
 * =========================================================================
 */

#include "precompiled.h"

#include "Patch.h"
#include "Terrain.h"


///////////////////////////////////////////////////////////////////////////////
// CPatch constructor
CPatch::CPatch()
: m_Parent(0), m_bWillBeDrawn(false)
{
}

///////////////////////////////////////////////////////////////////////////////
// CPatch destructor
CPatch::~CPatch()
{
	
}

///////////////////////////////////////////////////////////////////////////////
// Initialize: setup patch data
void CPatch::Initialize(CTerrain* parent,ssize_t x,ssize_t z)
{
	delete m_RenderData;
	m_RenderData=0;

	m_Parent=parent;
	m_X=x;
	m_Z=z;

	// set parent of each patch	
	for (ssize_t j=0;j<PATCH_SIZE;j++)
	{
		for (ssize_t i=0;i<PATCH_SIZE;i++)
		{
			m_MiniPatches[j][i].m_Parent=this;
		}
	}

	InvalidateBounds();
}

///////////////////////////////////////////////////////////////////////////////
// CalcBounds: calculating the bounds of this patch
void CPatch::CalcBounds()
{
	m_Bounds.SetEmpty();

	for (ssize_t j=0;j<PATCH_SIZE+1;j++)
	{
		for (ssize_t i=0;i<PATCH_SIZE+1;i++)
		{
			CVector3D pos;
			m_Parent->CalcPosition(m_X*PATCH_SIZE+i,m_Z*PATCH_SIZE+j,pos);
			m_Bounds+=pos;
		}
	}
}
