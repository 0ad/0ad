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

#include "precompiled.h"

#include "OverlayRenderer.h"

#include "graphics/LOSTexture.h"
#include "graphics/Overlay.h"
#include "graphics/ShaderManager.h"
#include "graphics/Terrain.h"
#include "graphics/TextureManager.h"
#include "lib/ogl.h"
#include "ps/Game.h"
#include "ps/Profile.h"
#include "renderer/Renderer.h"
#include "renderer/VertexBuffer.h"
#include "renderer/VertexBufferManager.h"
#include "simulation2/Simulation2.h"
#include "simulation2/components/ICmpWaterManager.h"

struct OverlayRendererInternals
{
	std::vector<SOverlayLine*> lines;
	std::vector<SOverlayTexturedLine*> texlines;
	std::vector<SOverlaySprite*> sprites;
};

class CTexturedLineRData : public CRenderData
{
public:
	CTexturedLineRData(SOverlayTexturedLine* line) :
		m_Line(line), m_VB(NULL), m_VBIndices(NULL)
	{
	}

	~CTexturedLineRData()
	{
		if (m_VB)
			g_VBMan.Release(m_VB);
		if (m_VBIndices)
			g_VBMan.Release(m_VBIndices);
	}

	struct SVertex
	{
		SVertex(CVector3D pos, short u, short v) : m_Position(pos) { m_UVs[0] = u; m_UVs[1] = v; }
		CVector3D m_Position;
		GLshort m_UVs[2];
	};
	cassert(sizeof(SVertex) == 16);

	void Update();

	SOverlayTexturedLine* m_Line;

	CVertexBuffer::VBChunk* m_VB;
	CVertexBuffer::VBChunk* m_VBIndices;
};

OverlayRenderer::OverlayRenderer()
{
	m = new OverlayRendererInternals();
}

OverlayRenderer::~OverlayRenderer()
{
	delete m;
}

void OverlayRenderer::Submit(SOverlayLine* line)
{
	ENSURE(line->m_Coords.size() % 3 == 0);

	m->lines.push_back(line);
}

void OverlayRenderer::Submit(SOverlayTexturedLine* line)
{
	// Simplify the rest of the code by guaranteeing non-empty lines
	if (line->m_Coords.empty())
		return;

	ENSURE(line->m_Coords.size() % 2 == 0);

	m->texlines.push_back(line);
}

void OverlayRenderer::Submit(SOverlaySprite* overlay)
{
	m->sprites.push_back(overlay);
}

void OverlayRenderer::EndFrame()
{
	m->lines.clear();
	m->texlines.clear();
	m->sprites.clear();
	// this should leave the capacity unchanged, which is okay since it
	// won't be very large or very variable
}

void OverlayRenderer::PrepareForRendering()
{
	PROFILE3("prepare overlays");

	// This is where we should do something like sort the overlays by
	// colour/sprite/etc for more efficient rendering

	for (size_t i = 0; i < m->texlines.size(); ++i)
	{
		SOverlayTexturedLine* line = m->texlines[i];
		if (!line->m_RenderData)
		{
			line->m_RenderData = shared_ptr<CRenderData>(new CTexturedLineRData(line));
			static_cast<CTexturedLineRData*>(line->m_RenderData.get())->Update();
			// We assume the overlay line will get replaced by the caller
			// if terrain changes, so we don't need to detect that here and
			// call Update again. Also we assume the caller won't change
			// any of the parameters after first submitting the line.
		}
	}
}

void OverlayRenderer::RenderOverlaysBeforeWater()
{
	PROFILE3_GPU("overlays (before)");

	glDisable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);

	for (size_t i = 0; i < m->lines.size(); ++i)
	{
		SOverlayLine* line = m->lines[i];
		if (line->m_Coords.empty())
			continue;

		ENSURE(line->m_Coords.size() % 3 == 0);

		glColor4fv(line->m_Color.FloatArray());
		glLineWidth((float)line->m_Thickness);

		glInterleavedArrays(GL_V3F, sizeof(float)*3, &line->m_Coords[0]);
		glDrawArrays(GL_LINE_STRIP, 0, (GLsizei)line->m_Coords.size()/3);
	}

	glDisableClientState(GL_VERTEX_ARRAY);

	glLineWidth(1.f);
	glDisable(GL_BLEND);
}

void OverlayRenderer::RenderOverlaysAfterWater()
{
	PROFILE3_GPU("overlays (after)");

	if (!m->texlines.empty())
	{
		glEnable(GL_TEXTURE_2D);
		glEnable(GL_BLEND);
		glDepthMask(0);

		const char* shaderName;
		if (g_Renderer.GetRenderPath() == CRenderer::RP_SHADER)
			shaderName = "overlayline";
		else
			shaderName = "fixed:overlayline";

		CShaderManager& shaderManager = g_Renderer.GetShaderManager();
		CShaderProgramPtr shaderTexLine(shaderManager.LoadProgram(shaderName, std::map<CStr, CStr>()));

		shaderTexLine->Bind();

		int streamflags = shaderTexLine->GetStreamFlags();

		if (streamflags & STREAM_POS)
			glEnableClientState(GL_VERTEX_ARRAY);

		if (streamflags & STREAM_UV0)
		{
			pglClientActiveTextureARB(GL_TEXTURE0);
			glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		}

		if (streamflags & STREAM_UV1)
		{
			pglClientActiveTextureARB(GL_TEXTURE1);
			glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		}

		CLOSTexture& los = g_Renderer.GetScene().GetLOSTexture();
		shaderTexLine->BindTexture("losTex", los.GetTexture());
		shaderTexLine->Uniform("losTransform", los.GetTextureMatrix()[0], los.GetTextureMatrix()[12], 0.f, 0.f);

		for (size_t i = 0; i < m->texlines.size(); ++i)
		{
			SOverlayTexturedLine* line = m->texlines[i];
			if (!line->m_RenderData)
				continue;

			shaderTexLine->BindTexture("baseTex", line->m_TextureBase->GetHandle());
			shaderTexLine->BindTexture("maskTex", line->m_TextureMask->GetHandle());
			shaderTexLine->Uniform("objectColor", line->m_Color);

			CTexturedLineRData* rdata = static_cast<CTexturedLineRData*>(line->m_RenderData.get());

			GLsizei stride = sizeof(CTexturedLineRData::SVertex);
			CTexturedLineRData::SVertex* base = reinterpret_cast<CTexturedLineRData::SVertex*>(rdata->m_VB->m_Owner->Bind());

			if (streamflags & STREAM_POS)
				glVertexPointer(3, GL_FLOAT, stride, &base->m_Position[0]);

			if (streamflags & STREAM_UV0)
			{
				pglClientActiveTextureARB(GL_TEXTURE0);
				glTexCoordPointer(2, GL_SHORT, stride, &base->m_UVs[0]);
			}

			if (streamflags & STREAM_UV1)
			{
				pglClientActiveTextureARB(GL_TEXTURE1);
				glTexCoordPointer(2, GL_SHORT, stride, &base->m_UVs[0]);
			}

			u8* indexBase = rdata->m_VBIndices->m_Owner->Bind();
			glDrawElements(GL_QUAD_STRIP, rdata->m_VBIndices->m_Count, GL_UNSIGNED_SHORT, indexBase + sizeof(u16)*rdata->m_VBIndices->m_Index);

			g_Renderer.GetStats().m_OverlayTris += rdata->m_VBIndices->m_Count - 2;
		}

		shaderTexLine->Unbind();

		// TODO: the shader should probably be responsible for unbinding its textures
		g_Renderer.BindTexture(1, 0);
		g_Renderer.BindTexture(0, 0);

		CVertexBuffer::Unbind();
		glDisableClientState(GL_VERTEX_ARRAY);
		pglClientActiveTextureARB(GL_TEXTURE1);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		pglClientActiveTextureARB(GL_TEXTURE0);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);

		glDepthMask(1);
		glDisable(GL_BLEND);
	}
}

void OverlayRenderer::RenderForegroundOverlays(const CCamera& viewCamera)
{
	PROFILE3_GPU("overlays (fg)");

	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);

	CVector3D right = -viewCamera.m_Orientation.GetLeft();
	CVector3D up = viewCamera.m_Orientation.GetUp();

	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	float uvs[8] = { 0,0, 1,0, 1,1, 0,1 };
	glTexCoordPointer(2, GL_FLOAT, sizeof(float)*2, &uvs);

	for (size_t i = 0; i < m->sprites.size(); ++i)
	{
		SOverlaySprite* sprite = m->sprites[i];

		sprite->m_Texture->Bind();

		CVector3D pos[4] = {
			sprite->m_Position + right*sprite->m_X0 + up*sprite->m_Y0,
			sprite->m_Position + right*sprite->m_X1 + up*sprite->m_Y0,
			sprite->m_Position + right*sprite->m_X1 + up*sprite->m_Y1,
			sprite->m_Position + right*sprite->m_X0 + up*sprite->m_Y1
		};

		glVertexPointer(3, GL_FLOAT, sizeof(float)*3, &pos[0].X);

		glDrawArrays(GL_QUADS, 0, (GLsizei)4);
	}

	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);

	glEnable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);
	glDisable(GL_TEXTURE_2D);
}

void CTexturedLineRData::Update()
{
	if (m_VB)
	{
		g_VBMan.Release(m_VB);
		m_VB = NULL;
	}

	if (m_VBIndices)
	{
		g_VBMan.Release(m_VBIndices);
		m_VBIndices = NULL;
	}

	CmpPtr<ICmpWaterManager> cmpWaterManager(*g_Game->GetSimulation2(), SYSTEM_ENTITY);

	std::vector<SVertex> vertices;
	std::vector<u16> indices;

	short v = 0;

	size_t n = m_Line->m_Coords.size() / 2;
	ENSURE(n >= 1);

	CTerrain* terrain = m_Line->m_Terrain;

	// TODO: this assumes paths are closed loops; probably should extend this to
	// handle non-closed paths too

	// In each iteration, p1 is the position of vertex i, p0 is i-1, p2 is i+1.
	// To avoid slightly expensive terrain computations we cycle these around and
	// recompute p2 at the end of each iteration.
	CVector3D p0 = CVector3D(m_Line->m_Coords[(n-1)*2], 0, m_Line->m_Coords[(n-1)*2+1]);
	CVector3D p1 = CVector3D(m_Line->m_Coords[0], 0, m_Line->m_Coords[1]);
	CVector3D p2 = CVector3D(m_Line->m_Coords[(1 % n)*2], 0, m_Line->m_Coords[(1 % n)*2+1]);
	bool p1floating = false;
	bool p2floating = false;

	// Compute terrain heights, clamped to the water height (and remember whether
	// each point was floating on water, for normal computation later)

	// TODO: if we ever support more than one water level per map, recompute this per point
	float w = cmpWaterManager->GetExactWaterLevel(p0.X, p0.Z);

	p0.Y = terrain->GetExactGroundLevel(p0.X, p0.Z);
	if (p0.Y < w)
		p0.Y = w;

	p1.Y = terrain->GetExactGroundLevel(p1.X, p1.Z);
	if (p1.Y < w)
	{
		p1.Y = w;
		p1floating = true;
	}

	p2.Y = terrain->GetExactGroundLevel(p2.X, p2.Z);
	if (p2.Y < w)
	{
		p2.Y = w;
		p2floating = true;
	}

	for (size_t i = 0; i < n; ++i)
	{
		// For vertex i, compute bisector of lines (i-1)..(i) and (i)..(i+1)
		// perpendicular to terrain normal

		// Normal is vertical if on water, else computed from terrain
		CVector3D norm;
		if (p1floating)
			norm = CVector3D(0, 1, 0);
		else
			norm = m_Line->m_Terrain->CalcExactNormal(p1.X, p1.Z);

		CVector3D b = ((p1 - p0).Normalized() + (p2 - p1).Normalized()).Cross(norm);

		// Adjust bisector length to match the line thickness, along the line's width
		float l = b.Dot((p2 - p1).Normalized().Cross(norm));
		if (fabs(l) > 0.000001f) // avoid unlikely divide-by-zero
			b *= m_Line->m_Thickness / l;

		// Raise off the terrain a little bit
		const float raised = 0.2f;

		vertices.push_back(SVertex(p1 + b + norm*raised, 0, v));
		indices.push_back(vertices.size() - 1);

		vertices.push_back(SVertex(p1 - b + norm*raised, 1, v));
		indices.push_back(vertices.size() - 1);

		// Alternate V coordinate for debugging
		v = 1 - v;

		// Cycle the p's and compute the new p2
		p0 = p1;
		p1 = p2;
		p1floating = p2floating;
		p2 = CVector3D(m_Line->m_Coords[((i+2) % n)*2], 0, m_Line->m_Coords[((i+2) % n)*2+1]);
		p2.Y = terrain->GetExactGroundLevel(p2.X, p2.Z);
		if (p2.Y < w)
		{
			p2.Y = w;
			p2floating = true;
		}
		else
			p2floating = false;
	}

	// Close the path
	indices.push_back(0);
	indices.push_back(1);

	m_VB = g_VBMan.Allocate(sizeof(SVertex), vertices.size(), GL_STATIC_DRAW, GL_ARRAY_BUFFER);
	m_VB->m_Owner->UpdateChunkVertices(m_VB, &vertices[0]);

	// Update the indices to include the base offset of the vertex data
	for (size_t k = 0; k < indices.size(); ++k)
		indices[k] += m_VB->m_Index;

	m_VBIndices = g_VBMan.Allocate(sizeof(u16), indices.size(), GL_STATIC_DRAW, GL_ELEMENT_ARRAY_BUFFER);
	m_VBIndices->m_Owner->UpdateChunkVertices(m_VBIndices, &indices[0]);
}
