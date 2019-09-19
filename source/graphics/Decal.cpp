/* Copyright (C) 2019 Wildfire Games.
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

#include "precompiled.h"

#include "Decal.h"

#include "graphics/Terrain.h"
#include "maths/MathUtil.h"

CModelAbstract* CModelDecal::Clone() const
{
	CModelDecal* clone = new CModelDecal(m_Terrain, m_Decal);
	return clone;
}

void CModelDecal::CalcVertexExtents(ssize_t& i0, ssize_t& j0, ssize_t& i1, ssize_t& j1)
{
	CVector3D corner0(m_Decal.m_OffsetX + m_Decal.m_SizeX/2, 0, m_Decal.m_OffsetZ + m_Decal.m_SizeZ/2);
	CVector3D corner1(m_Decal.m_OffsetX + m_Decal.m_SizeX/2, 0, m_Decal.m_OffsetZ - m_Decal.m_SizeZ/2);
	CVector3D corner2(m_Decal.m_OffsetX - m_Decal.m_SizeX/2, 0, m_Decal.m_OffsetZ - m_Decal.m_SizeZ/2);
	CVector3D corner3(m_Decal.m_OffsetX - m_Decal.m_SizeX/2, 0, m_Decal.m_OffsetZ + m_Decal.m_SizeZ/2);

	corner0 = GetTransform().Transform(corner0);
	corner1 = GetTransform().Transform(corner1);
	corner2 = GetTransform().Transform(corner2);
	corner3 = GetTransform().Transform(corner3);

	i0 = floor(std::min(std::min(corner0.X, corner1.X), std::min(corner2.X, corner3.X)) / TERRAIN_TILE_SIZE);
	j0 = floor(std::min(std::min(corner0.Z, corner1.Z), std::min(corner2.Z, corner3.Z)) / TERRAIN_TILE_SIZE);
	i1 = ceil(std::max(std::max(corner0.X, corner1.X), std::max(corner2.X, corner3.X)) / TERRAIN_TILE_SIZE);
	j1 = ceil(std::max(std::max(corner0.Z, corner1.Z), std::max(corner2.Z, corner3.Z)) / TERRAIN_TILE_SIZE);

	i0 = Clamp(i0, static_cast<ssize_t>(0), m_Terrain->GetVerticesPerSide() - 1);
	j0 = Clamp(j0, static_cast<ssize_t>(0), m_Terrain->GetVerticesPerSide() - 1);
	i1 = Clamp(i1, static_cast<ssize_t>(0), m_Terrain->GetVerticesPerSide() - 1);
	j1 = Clamp(j1, static_cast<ssize_t>(0), m_Terrain->GetVerticesPerSide() - 1);
}

void CModelDecal::CalcBounds()
{
	ssize_t i0, j0, i1, j1;
	CalcVertexExtents(i0, j0, i1, j1);
	m_WorldBounds = m_Terrain->GetVertexesBound(i0, j0, i1, j1);
}

void CModelDecal::SetTerrainDirty(ssize_t i0, ssize_t j0, ssize_t i1, ssize_t j1)
{
	// Check if there's no intersection between the dirty range and this decal
	ssize_t bi0, bj0, bi1, bj1;
	CalcVertexExtents(bi0, bj0, bi1, bj1);
	if (bi1 < i0 || bi0 > i1 || bj1 < j0 || bj0 > j1)
		return;

	SetDirty(RENDERDATA_UPDATE_VERTICES);
}

void CModelDecal::InvalidatePosition()
{
	m_PositionValid = false;
}

void CModelDecal::ValidatePosition()
{
	if (m_PositionValid)
	{
		ENSURE(!m_Parent || m_Parent->m_PositionValid);
		return;
	}

	if (m_Parent && !m_Parent->m_PositionValid)
	{
		// Make sure we don't base our calculations on
		// a parent animation state that is out of date.
		m_Parent->ValidatePosition();

		// Parent will recursively call our validation.
		ENSURE(m_PositionValid);
		return;
	}

	m_PositionValid = true;
}

void CModelDecal::SetTransform(const CMatrix3D& transform)
{
	// Since decals are assumed to be horizontal and projected downwards
	// onto the terrain, use just the Y-axis rotation and the translation
	CMatrix3D newTransform;
	newTransform.SetYRotation(transform.GetYRotation() + m_Decal.m_Angle);
	newTransform.Translate(transform.GetTranslation());

	CRenderableObject::SetTransform(newTransform);
	InvalidatePosition();
}

void CModelDecal::RemoveShadows()
{
	m_Decal.m_Material.AddShaderDefine(str_DISABLE_RECEIVE_SHADOWS, str_1);
	m_Decal.m_Material.RecomputeCombinedShaderDefines();
}
