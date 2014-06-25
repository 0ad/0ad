/* Copyright (C) 2010 Wildfire Games.
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

/*
 * A patch of terrain holding NxN MiniPatch tiles
 */

#include "precompiled.h"

#include "Patch.h"
#include "Terrain.h"


///////////////////////////////////////////////////////////////////////////////
// CPatch constructor
CPatch::CPatch()
: m_Parent(0)
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

	InvalidateBounds();
}

///////////////////////////////////////////////////////////////////////////////
// CalcBounds: calculating the bounds of this patch
void CPatch::CalcBounds()
{
	m_WorldBounds.SetEmpty();

	for (ssize_t j=0;j<PATCH_SIZE+1;j++)
	{
		for (ssize_t i=0;i<PATCH_SIZE+1;i++)
		{
			CVector3D pos;
			m_Parent->CalcPosition(m_X*PATCH_SIZE+i,m_Z*PATCH_SIZE+j,pos);
			m_WorldBounds+=pos;
		}
	}

	// If this a side patch, the sides go down to height 0, so add them
	// into the bounds
	if (GetSideFlags())
		m_WorldBounds[0].Y = std::min(m_WorldBounds[0].Y, 0.f);
}

int CPatch::GetSideFlags()
{
	int flags = 0;
	if (m_X == 0)
		flags |= CPATCH_SIDE_NEGX;
	if (m_Z == 0)
		flags |= CPATCH_SIDE_NEGZ;
	if (m_X == m_Parent->GetPatchesPerSide()-1)
		flags |= CPATCH_SIDE_POSX;
	if (m_Z == m_Parent->GetPatchesPerSide()-1)
		flags |= CPATCH_SIDE_POSZ;
	return flags;
}
