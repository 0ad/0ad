/* Copyright (C) 2011 Wildfire Games.
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

#include "DecalRData.h"

#include "graphics/Decal.h"
#include "graphics/LightEnv.h"
#include "graphics/Model.h"
#include "graphics/Terrain.h"
#include "graphics/TextureManager.h"
#include "ps/Game.h"
#include "ps/Profile.h"
#include "renderer/Renderer.h"
#include "simulation2/Simulation2.h"
#include "simulation2/components/ICmpWaterManager.h"

// TODO: Currently each decal is a separate CDecalRData. We might want to use
// lots of decals for special effects like shadows, footprints, etc, in which
// case we should probably redesign this to batch them all together for more
// efficient rendering.

CDecalRData::CDecalRData(CModelDecal* decal)
	: m_Decal(decal), m_IndexArray(GL_STATIC_DRAW), m_Array(GL_STATIC_DRAW)
{
	m_Position.type = GL_FLOAT;
	m_Position.elems = 3;
	m_Array.AddAttribute(&m_Position);

	m_DiffuseColor.type = GL_UNSIGNED_BYTE;
	m_DiffuseColor.elems = 4;
	m_Array.AddAttribute(&m_DiffuseColor);

	m_UV.type = GL_FLOAT;
	m_UV.elems = 2;
	m_Array.AddAttribute(&m_UV);

	BuildArrays();
}

CDecalRData::~CDecalRData()
{
}

void CDecalRData::Update()
{
	if (m_UpdateFlags != 0)
	{
		BuildArrays();
		m_UpdateFlags = 0;
	}
}

void CDecalRData::Render()
{
	m_Decal->m_Decal.m_Texture->Bind(0);

	// TODO: Need to handle floating decals correctly. In particular, we need
	// to render non-floating before water and floating after water (to get
	// the blending right), and we also need to apply the correct lighting in
	// each case, which doesn't really seem possible with the current
	// TerrainRenderer.
	// Also, need to mark the decals as dirty when water height changes.

//	glDisable(GL_TEXTURE_2D);
//	m_Decal->GetBounds().Render();
//	glEnable(GL_TEXTURE_2D);

	u8* base = m_Array.Bind();
	GLsizei stride = (GLsizei)m_Array.GetStride();

	u8* indexBase = m_IndexArray.Bind();

	// TODO: make the shading color available to shader-based rendering
	// (which uses the color array) too
	glColor3fv(m_Decal->GetShadingColor().FloatArray());

	glVertexPointer(3, GL_FLOAT, stride, base + m_Position.offset);
	glColorPointer(4, GL_UNSIGNED_BYTE, stride, base + m_DiffuseColor.offset);
	glTexCoordPointer(2, GL_FLOAT, stride, base + m_UV.offset);

	if (!g_Renderer.m_SkipSubmit)
	{
		glDrawElements(GL_TRIANGLES, (GLsizei)m_IndexArray.GetNumVertices(), GL_UNSIGNED_SHORT, indexBase);
	}

	// bump stats
	g_Renderer.m_Stats.m_DrawCalls++;
	g_Renderer.m_Stats.m_TerrainTris += m_IndexArray.GetNumVertices() / 3;

	CVertexBuffer::Unbind();
}

void CDecalRData::BuildArrays()
{
	PROFILE("decal build");

	const SDecal& decal = m_Decal->m_Decal;

	// TODO: Currently this constructs an axis-aligned bounding rectangle around
	// the decal. It would be more efficient for rendering if we excluded tiles
	// that are outside the (non-axis-aligned) decal rectangle.

	ssize_t i0, j0, i1, j1;
	m_Decal->CalcVertexExtents(i0, j0, i1, j1);

	// Construct vertex data arrays

	CmpPtr<ICmpWaterManager> cmpWaterManager(*g_Game->GetSimulation2(), SYSTEM_ENTITY);

	m_Array.SetNumVertices((i1-i0+1)*(j1-j0+1));
	m_Array.Layout();
	VertexArrayIterator<CVector3D> Position = m_Position.GetIterator<CVector3D>();
	VertexArrayIterator<SColor4ub> DiffuseColor = m_DiffuseColor.GetIterator<SColor4ub>();
	VertexArrayIterator<float[2]> UV = m_UV.GetIterator<float[2]>();

	bool includeSunColor = (g_Renderer.GetRenderPath() != CRenderer::RP_SHADER);

	for (ssize_t j = j0; j <= j1; ++j)
	{
		for (ssize_t i = i0; i <= i1; ++i)
		{
			CVector3D pos;
			m_Decal->m_Terrain->CalcPosition(i, j, pos);

			if (decal.m_Floating && !cmpWaterManager.null())
				pos.Y = std::max(pos.Y, cmpWaterManager->GetExactWaterLevel(pos.X, pos.Z));

			*Position = pos;
			++Position;

			CVector3D normal;
			m_Decal->m_Terrain->CalcNormal(i, j, normal);
			*DiffuseColor = g_Renderer.GetLightEnv().EvaluateDiffuse(normal, includeSunColor);
			++DiffuseColor;

			// Map from world space back into decal texture space
			CVector3D inv = m_Decal->GetInvTransform().Transform(pos);
			(*UV)[0] = 0.5f + (inv.X - decal.m_OffsetX) / decal.m_SizeX;
			(*UV)[1] = 0.5f - (inv.Z - decal.m_OffsetZ) / decal.m_SizeZ; // flip V to match our texture convention
			++UV;
		}
	}

	m_Array.Upload();
	m_Array.FreeBackingStore();

	// Construct index arrays for each terrain tile

	m_IndexArray.SetNumVertices((i1-i0)*(j1-j0)*6);
	m_IndexArray.Layout();
	VertexArrayIterator<u16> Index = m_IndexArray.GetIterator();

	u16 base = 0;
	ssize_t w = i1-i0+1;
	for (ssize_t dj = 0; dj < j1-j0; ++dj)
	{
		for (ssize_t di = 0; di < i1-i0; ++di)
		{
			bool dir = m_Decal->m_Terrain->GetTriangulationDir(i0+di, j0+dj);
			if (dir)
			{
				*Index++ = u16(((dj+0)*w+(di+0))+base);
				*Index++ = u16(((dj+0)*w+(di+1))+base);
				*Index++ = u16(((dj+1)*w+(di+0))+base);

				*Index++ = u16(((dj+0)*w+(di+1))+base);
				*Index++ = u16(((dj+1)*w+(di+1))+base);
				*Index++ = u16(((dj+1)*w+(di+0))+base);
			}
			else
			{
				*Index++ = u16(((dj+0)*w+(di+0))+base);
				*Index++ = u16(((dj+0)*w+(di+1))+base);
				*Index++ = u16(((dj+1)*w+(di+1))+base);

				*Index++ = u16(((dj+1)*w+(di+1))+base);
				*Index++ = u16(((dj+1)*w+(di+0))+base);
				*Index++ = u16(((dj+0)*w+(di+0))+base);
			}
		}
	}

	m_IndexArray.Upload();
	m_IndexArray.FreeBackingStore();
}
