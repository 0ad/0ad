/* Copyright (C) 2012 Wildfire Games.
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

#include <boost/unordered_map.hpp>
#include "graphics/LOSTexture.h"
#include "graphics/Overlay.h"
#include "graphics/Terrain.h"
#include "graphics/TextureManager.h"
#include "lib/ogl.h"
#include "maths/MathUtil.h"
#include "maths/Quaternion.h"
#include "ps/Game.h"
#include "ps/Profile.h"
#include "renderer/Renderer.h"
#include "renderer/VertexArray.h"
#include "renderer/VertexBuffer.h"
#include "renderer/VertexBufferManager.h"
#include "simulation2/Simulation2.h"
#include "simulation2/components/ICmpWaterManager.h"
#include "simulation2/system/SimContext.h"

/**
 * As a general TODO, some of the code here still uses g_VBMan manually.
 * For consistency with other parts of the engine, it'd be nice to switch
 * over to the cleaner and more readable VertexArray API.
 */

/**
 * Key used to group quads into batches for more efficient rendering. Currently groups by the combination
 * of the main texture and the texture mask, to minimize texture swapping during rendering.
 */
struct QuadBatchKey
{
	QuadBatchKey (const CTexturePtr& texture, const CTexturePtr& textureMask)
		: m_Texture(texture), m_TextureMask(textureMask)
	{ }

	bool operator==(const QuadBatchKey& other) const
	{
		return (m_Texture == other.m_Texture && m_TextureMask == other.m_TextureMask);
	}

	CTexturePtr m_Texture;
	CTexturePtr m_TextureMask;
};

/**
 * Holds information about a single quad rendering batch.
 */
class QuadBatchData : public CRenderData
{
public:
	QuadBatchData() : m_IndicesBase(0), m_NumRenderQuads(0) { }

	/// Holds the quad overlay structures requested to be rendered in this batch. Must be cleared
	/// after each frame.
	std::vector<SOverlayQuad*> m_Quads;

	/// Start index of this batch into the dedicated quad indices VertexArray (see OverlayInternals).
	size_t m_IndicesBase;
	/// Amount of quads to actually render in this batch. Potentially (although unlikely to be)
	/// different from m_Quads.size() due to restrictions on the total amount of quads that can be
	/// rendered. Must be reset after each frame.
	size_t m_NumRenderQuads;
};

struct OverlayRendererInternals
{
	typedef boost::unordered_map<QuadBatchKey, QuadBatchData> QuadBatchMap;

	OverlayRendererInternals();
	~OverlayRendererInternals(){ }

	std::vector<SOverlayLine*> lines;
	std::vector<SOverlayTexturedLine*> texlines;
	std::vector<SOverlaySprite*> sprites;
	std::vector<SOverlayQuad*> quads;

	QuadBatchMap quadBatchMap;

	// Dedicated vertex/index buffers for rendering all quads (to within the limits set by
	// MAX_QUAD_OVERLAYS).
	VertexArray quadVertices;
	VertexArray::Attribute quadAttributePos;
	VertexArray::Attribute quadAttributeColor;
	VertexArray::Attribute quadAttributeUV;
	VertexIndexArray quadIndices;

	/// Maximum amount of quad overlays we support for rendering. This limit is set to be able to 
	/// render all quads from a single dedicated VB without having to reallocate it, which is much
	/// faster in the typical case of rendering only a handful of quads. When modifying this value,
	/// you must take care for the new amount of quads to fit in a single VBO (which is not likely
	/// to be a problem).
	static const size_t MAX_QUAD_OVERLAYS = 1024;

	// Sets of commonly-(re)used shader defines.
	CShaderDefines defsOverlayLineNormal;
	CShaderDefines defsOverlayLineAlwaysVisible;
	CShaderDefines defsQuadOverlay;

	/// Small vertical offset of overlays from terrain to prevent visual glitches
	static const float OVERLAY_VOFFSET;

	/// Performs one-time setup. Called from CRenderer::Open, after graphics capabilities have
	/// been detected. Note that no VBOs must be created before this is called, since the shader
	/// path and graphics capabilities are not guaranteed to be stable before this point.
	void Initialize();
};

const float OverlayRendererInternals::OVERLAY_VOFFSET = 0.2f;

OverlayRendererInternals::OverlayRendererInternals()
	: quadVertices(GL_DYNAMIC_DRAW), quadIndices(GL_DYNAMIC_DRAW)
{
	quadAttributePos.elems = 3;
	quadAttributePos.type = GL_FLOAT;
	quadVertices.AddAttribute(&quadAttributePos);

	quadAttributeColor.elems = 4;
	quadAttributeColor.type = GL_FLOAT;
	quadVertices.AddAttribute(&quadAttributeColor);

	quadAttributeUV.elems = 2;
	quadAttributeUV.type = GL_SHORT; // don't use GL_UNSIGNED_SHORT here, TexCoordPointer won't accept it
	quadVertices.AddAttribute(&quadAttributeUV);

	// Note that we're reusing the textured overlay line shader for the quad overlay rendering. This
	// is because their code is almost identical; the only difference is that for the quad overlays
	// we want to use a vertex color stream as opposed to an objectColor uniform. To this end, the
	// shader has been set up to switch between the two behaviours based on the USE_OBJECTCOLOR define.
	defsOverlayLineNormal.Add("USE_OBJECTCOLOR", "1");
	defsOverlayLineAlwaysVisible.Add("USE_OBJECTCOLOR", "1");
	defsOverlayLineAlwaysVisible.Add("IGNORE_LOS", "1");
}

void OverlayRendererInternals::Initialize()
{
	// Perform any initialization after graphics capabilities have been detected. Notably,
	// only at this point can we safely allocate VBOs (in contrast to e.g. in the constructor),
	// because their creation depends on the shader path, which is not reliably set before this point.
	
	quadVertices.SetNumVertices(MAX_QUAD_OVERLAYS * 4);
	quadVertices.Layout(); // allocate backing store

	quadIndices.SetNumVertices(MAX_QUAD_OVERLAYS * 6);
	quadIndices.Layout(); // allocate backing store

	// Since the quads in the vertex array are independent and always consist of exactly 4 vertices per quad, the
	// indices are always the same; we can therefore fill in all the indices once and pretty much forget about
	// them. We then also no longer need its backing store, since we never change any indices afterwards.
	VertexArrayIterator<u16> index = quadIndices.GetIterator();
	for (size_t i = 0; i < MAX_QUAD_OVERLAYS; ++i)
	{
		*index++ = i*4 + 0;
		*index++ = i*4 + 1;
		*index++ = i*4 + 2;
		*index++ = i*4 + 2;
		*index++ = i*4 + 3;
		*index++ = i*4 + 0;
	}
	quadIndices.Upload();
	quadIndices.FreeBackingStore();
}

class CTexturedLineRData : public CRenderData
{
public:
	CTexturedLineRData(SOverlayTexturedLine* line) : m_Line(line), m_VB(NULL), m_VBIndices(NULL)
	{ }

	~CTexturedLineRData()
	{
		if (m_VB)
			g_VBMan.Release(m_VB);
		if (m_VBIndices)
			g_VBMan.Release(m_VBIndices);
	}

	struct SVertex
	{
		SVertex(CVector3D pos, float u, float v) : m_Position(pos) { m_UVs[0] = u; m_UVs[1] = v; }
		CVector3D m_Position;
		GLfloat m_UVs[2];
		float _padding[3]; // get a pow2 struct size
	};
	cassert(sizeof(SVertex) == 32);

	void Update();

	/**
	 * Creates a line cap of the specified type @p endCapType at the end of the segment going in direction @p normal, and appends
	 * the vertices to @p verticesOut in GL_TRIANGLES order.
	 *
	 * @param corner1 One of the two butt-end corner points of the line to which the cap should be attached.
	 * @param corner2 One of the two butt-end corner points of the line to which the cap should be attached.
	 * @param normal Normal vector indicating the direction of the segment to which the cap should be attached.
	 * @param endCapType The type of end cap to produce.
	 * @param verticesOut Output vector of vertices for passing to the renderer.
	 * @param indicesOut Output vector of vertex indices for passing to the renderer.
	 */
	void CreateLineCap(const CVector3D& corner1, const CVector3D& corner2, const CVector3D& normal,
		               SOverlayTexturedLine::LineCapType endCapType, std::vector<SVertex>& verticesOut, std::vector<u16>& indicesOut);

	/// Small utility function; grabs the centroid of the positions of two vertices
	inline CVector3D Centroid(const SVertex& v1, const SVertex& v2)
	{
		return (v1.m_Position + v2.m_Position) * 0.5;
	}

	SOverlayTexturedLine* m_Line;
	CVertexBuffer::VBChunk* m_VB;
	CVertexBuffer::VBChunk* m_VBIndices;
};

static size_t hash_value(const QuadBatchKey& d)
{
	size_t seed = 0;
	boost::hash_combine(seed, d.m_Texture);
	boost::hash_combine(seed, d.m_TextureMask);
	return seed;
}

OverlayRenderer::OverlayRenderer()
{
	m = new OverlayRendererInternals();
}

OverlayRenderer::~OverlayRenderer()
{
	delete m;
}

void OverlayRenderer::Initialize()
{
	m->Initialize();
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

void OverlayRenderer::Submit(SOverlayQuad* overlay)
{
	m->quads.push_back(overlay);
}

void OverlayRenderer::EndFrame()
{
	m->lines.clear();
	m->texlines.clear();
	m->sprites.clear();
	m->quads.clear();
	// this should leave the capacity unchanged, which is okay since it
	// won't be very large or very variable
	
	// Empty the batch rendering data structures, but keep their key mappings around for the next frames
	for (OverlayRendererInternals::QuadBatchMap::iterator it = m->quadBatchMap.begin(); it != m->quadBatchMap.end(); it++)
	{
		QuadBatchData& quadBatchData = (it->second);
		quadBatchData.m_Quads.clear();
		quadBatchData.m_NumRenderQuads = 0;
		quadBatchData.m_IndicesBase = 0;
	}
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

	// Group quad overlays by their texture/mask combination for efficient rendering
	// TODO: consider doing this directly in Submit()
	for (size_t i = 0; i < m->quads.size(); ++i)
	{
		SOverlayQuad* const quad = m->quads[i];

		QuadBatchKey textures(quad->m_Texture, quad->m_TextureMask);
		QuadBatchData& batchRenderData = m->quadBatchMap[textures]; // will create entry if it doesn't already exist

		// add overlay to list of quads
		batchRenderData.m_Quads.push_back(quad);
	}

	const CVector3D vOffset(0, OverlayRendererInternals::OVERLAY_VOFFSET, 0);

	// Write quad overlay vertices/indices to VA backing store
	VertexArrayIterator<CVector3D> vertexPos = m->quadAttributePos.GetIterator<CVector3D>();
	VertexArrayIterator<CVector4D> vertexColor = m->quadAttributeColor.GetIterator<CVector4D>();
	VertexArrayIterator<short[2]> vertexUV = m->quadAttributeUV.GetIterator<short[2]>();

	size_t indicesIdx = 0;
	size_t totalNumQuads = 0;

	for (OverlayRendererInternals::QuadBatchMap::iterator it = m->quadBatchMap.begin(); it != m->quadBatchMap.end(); ++it)
	{
		QuadBatchData& batchRenderData = (it->second);
		batchRenderData.m_NumRenderQuads = 0;

		if (batchRenderData.m_Quads.empty())
			continue;

		// Remember the current index into the (entire) indices array as our base offset for this batch
		batchRenderData.m_IndicesBase = indicesIdx;

		// points to the index where each iteration's vertices will be appended
		for (size_t i = 0; i < batchRenderData.m_Quads.size() && totalNumQuads < OverlayRendererInternals::MAX_QUAD_OVERLAYS; i++)
		{
			const SOverlayQuad* quad = batchRenderData.m_Quads[i];

			// TODO: this is kind of ugly, the iterator should use a type that can have quad->m_Color assigned
			// to it directly
			const CVector4D quadColor(quad->m_Color.r, quad->m_Color.g, quad->m_Color.b, quad->m_Color.a);

			*vertexPos++ = quad->m_Corners[0] + vOffset;
			*vertexPos++ = quad->m_Corners[1] + vOffset;
			*vertexPos++ = quad->m_Corners[2] + vOffset;
			*vertexPos++ = quad->m_Corners[3] + vOffset;
			
			(*vertexUV)[0] = 0;
			(*vertexUV)[1] = 0;
			++vertexUV;
			(*vertexUV)[0] = 0;
			(*vertexUV)[1] = 1;
			++vertexUV;
			(*vertexUV)[0] = 1;
			(*vertexUV)[1] = 1;
			++vertexUV;
			(*vertexUV)[0] = 1;
			(*vertexUV)[1] = 0;
			++vertexUV;

			*vertexColor++ = quadColor;
			*vertexColor++ = quadColor;
			*vertexColor++ = quadColor;
			*vertexColor++ = quadColor;

			indicesIdx += 6;

			totalNumQuads++;
			batchRenderData.m_NumRenderQuads++;
		}
	}

	m->quadVertices.Upload();
	// don't free the backing store! we'll overwrite it on the next frame to save a reallocation.
}

void OverlayRenderer::RenderOverlaysBeforeWater()
{
	PROFILE3_GPU("overlays (before)");

#if CONFIG2_GLES
#warning TODO: implement OverlayRenderer::RenderOverlaysBeforeWater for GLES
#else
	pglActiveTextureARB(GL_TEXTURE0);
	glDisable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);

	// Ignore z so that we draw behind terrain (but don't disable GL_DEPTH_TEST
	// since we still want to write to the z buffer)
	glDepthFunc(GL_ALWAYS);

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
	glDepthFunc(GL_LEQUAL);
	glDisable(GL_BLEND);
#endif
}

void OverlayRenderer::RenderOverlaysAfterWater()
{
	PROFILE3_GPU("overlays (after)");

	RenderTexturedOverlayLines();
	RenderQuadOverlays();
}

void OverlayRenderer::RenderTexturedOverlayLines()
{
#if CONFIG2_GLES
#warning TODO: implement OverlayRenderer::RenderTexturedOverlayLines for GLES
	return;
#endif
	if (m->texlines.empty())
		return;

	ogl_WarnIfError();

	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glDepthMask(0);

	const char* shaderName;
	if (g_Renderer.GetRenderPath() == CRenderer::RP_SHADER)
		shaderName = "arb/overlayline";
	else
		shaderName = "fixed:overlayline";

	CLOSTexture& los = g_Renderer.GetScene().GetLOSTexture();

	CShaderManager& shaderManager = g_Renderer.GetShaderManager();
	CShaderProgramPtr shaderTexLineNormal(shaderManager.LoadProgram(shaderName, m->defsOverlayLineNormal));
	CShaderProgramPtr shaderTexLineAlwaysVisible(shaderManager.LoadProgram(shaderName, m->defsOverlayLineAlwaysVisible));

	// ----------------------------------------------------------------------------------------

	if (shaderTexLineNormal)
	{
		shaderTexLineNormal->Bind();
		shaderTexLineNormal->BindTexture("losTex", los.GetTexture());
		shaderTexLineNormal->Uniform("losTransform", los.GetTextureMatrix()[0], los.GetTextureMatrix()[12], 0.f, 0.f);

		// batch render only the non-always-visible overlay lines using the normal shader
		RenderTexturedOverlayLines(shaderTexLineNormal, false);

		shaderTexLineNormal->Unbind();
	}

	// ----------------------------------------------------------------------------------------

	if (shaderTexLineAlwaysVisible)
	{
		shaderTexLineAlwaysVisible->Bind();
		// TODO: losTex and losTransform are unused in the always visible shader; see if these can be safely omitted
		shaderTexLineAlwaysVisible->BindTexture("losTex", los.GetTexture());
		shaderTexLineAlwaysVisible->Uniform("losTransform", los.GetTextureMatrix()[0], los.GetTextureMatrix()[12], 0.f, 0.f);

		// batch render only the always-visible overlay lines using the LoS-ignored shader
		RenderTexturedOverlayLines(shaderTexLineAlwaysVisible, true);

		shaderTexLineAlwaysVisible->Unbind();
	}

	// ----------------------------------------------------------------------------------------

	// TODO: the shaders should probably be responsible for unbinding their textures
	g_Renderer.BindTexture(1, 0);
	g_Renderer.BindTexture(0, 0);

	CVertexBuffer::Unbind();

	glDepthMask(1);
	glDisable(GL_BLEND);
}

void OverlayRenderer::RenderTexturedOverlayLines(CShaderProgramPtr shaderTexLine, bool alwaysVisible)
{
	int streamflags = shaderTexLine->GetStreamFlags();

	for (size_t i = 0; i < m->texlines.size(); ++i)
	{
		SOverlayTexturedLine* line = m->texlines[i];

		// render only those lines matching the requested alwaysVisible status
		if (!line->m_RenderData || line->m_AlwaysVisible != alwaysVisible)
			continue;

		shaderTexLine->BindTexture("baseTex", line->m_TextureBase->GetHandle());
		shaderTexLine->BindTexture("maskTex", line->m_TextureMask->GetHandle());
		shaderTexLine->Uniform("objectColor", line->m_Color);

		CTexturedLineRData* rdata = static_cast<CTexturedLineRData*>(line->m_RenderData.get());
		if (!rdata->m_VB || !rdata->m_VBIndices)
			continue; // might have failed to allocate

		// -- render main line quad strip ----------------------

		GLsizei stride = sizeof(CTexturedLineRData::SVertex);
		CTexturedLineRData::SVertex* vertexBase = reinterpret_cast<CTexturedLineRData::SVertex*>(rdata->m_VB->m_Owner->Bind());

		if (streamflags & STREAM_POS)
			shaderTexLine->VertexPointer(3, GL_FLOAT, stride, &vertexBase->m_Position[0]);

		if (streamflags & STREAM_UV0)
			shaderTexLine->TexCoordPointer(GL_TEXTURE0, 2, GL_FLOAT, stride, &vertexBase->m_UVs[0]);

		if (streamflags & STREAM_UV1)
			shaderTexLine->TexCoordPointer(GL_TEXTURE1, 2, GL_FLOAT, stride, &vertexBase->m_UVs[0]);

		u8* indexBase = rdata->m_VBIndices->m_Owner->Bind();

		shaderTexLine->AssertPointersBound();
		glDrawElements(GL_TRIANGLES, rdata->m_VBIndices->m_Count, GL_UNSIGNED_SHORT, indexBase + sizeof(u16)*rdata->m_VBIndices->m_Index); 

		g_Renderer.GetStats().m_DrawCalls++;
		g_Renderer.GetStats().m_OverlayTris += rdata->m_VBIndices->m_Count/3; 
	}
}

void OverlayRenderer::RenderQuadOverlays()
{
	if (m->quadBatchMap.empty())
		return;

	ogl_WarnIfError();

	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glDepthMask(0);

	const char* shaderName;
	if (g_Renderer.GetRenderPath() == CRenderer::RP_SHADER)
		shaderName = "arb/overlayline";
	else
		shaderName = "fixed:overlayline";

	CLOSTexture& los = g_Renderer.GetScene().GetLOSTexture();

	CShaderManager& shaderManager = g_Renderer.GetShaderManager();
	CShaderProgramPtr shader(shaderManager.LoadProgram(shaderName, m->defsQuadOverlay));

	// ----------------------------------------------------------------------------------------

	if (shader)
	{
		shader->Bind();
		shader->BindTexture("losTex", los.GetTexture());
		shader->Uniform("losTransform", los.GetTextureMatrix()[0], los.GetTextureMatrix()[12], 0.f, 0.f);

		// Base offsets (in bytes) of the two backing stores relative to their owner VBO
		u8* indexBase = m->quadIndices.Bind();
		u8* vertexBase = m->quadVertices.Bind();
		GLsizei indexStride = m->quadIndices.GetStride();
		GLsizei vertexStride = m->quadVertices.GetStride();

		for (OverlayRendererInternals::QuadBatchMap::iterator it = m->quadBatchMap.begin(); it != m->quadBatchMap.end(); it++)
		{
			QuadBatchData& batchRenderData = it->second;
			const size_t batchNumQuads = batchRenderData.m_NumRenderQuads;

			// Careful; some drivers don't like drawing calls with 0 stuff to draw.
			if (batchNumQuads == 0)
				continue;

			const QuadBatchKey& maskPair = it->first;

			shader->BindTexture("baseTex", maskPair.m_Texture->GetHandle());
			shader->BindTexture("maskTex", maskPair.m_TextureMask->GetHandle());

			int streamflags = shader->GetStreamFlags();

			if (streamflags & STREAM_POS)
				shader->VertexPointer(m->quadAttributePos.elems, m->quadAttributePos.type, vertexStride, vertexBase + m->quadAttributePos.offset);

			if (streamflags & STREAM_UV0)
				shader->TexCoordPointer(GL_TEXTURE0, m->quadAttributeUV.elems, m->quadAttributeUV.type, vertexStride, vertexBase + m->quadAttributeUV.offset);

			if (streamflags & STREAM_UV1)
				shader->TexCoordPointer(GL_TEXTURE1, m->quadAttributeUV.elems, m->quadAttributeUV.type, vertexStride, vertexBase + m->quadAttributeUV.offset);
			
			if (streamflags & STREAM_COLOR)
				shader->ColorPointer(m->quadAttributeColor.elems, m->quadAttributeColor.type, vertexStride, vertexBase + m->quadAttributeColor.offset);
			
			shader->AssertPointersBound();
			glDrawElements(GL_TRIANGLES, (GLsizei)(batchNumQuads * 6), GL_UNSIGNED_SHORT, indexBase + indexStride * batchRenderData.m_IndicesBase);

			g_Renderer.GetStats().m_DrawCalls++;
			g_Renderer.GetStats().m_OverlayTris += batchNumQuads*2;
		}

		shader->Unbind();
	}

	// ----------------------------------------------------------------------------------------

	// TODO: the shader should probably be responsible for unbinding its textures
	g_Renderer.BindTexture(1, 0);
	g_Renderer.BindTexture(0, 0);

	CVertexBuffer::Unbind();

	glDepthMask(1);
	glDisable(GL_BLEND);
}

void OverlayRenderer::RenderForegroundOverlays(const CCamera& viewCamera)
{
	PROFILE3_GPU("overlays (fg)");

#if CONFIG2_GLES
#warning TODO: implement OverlayRenderer::RenderForegroundOverlays for GLES
#else
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

		g_Renderer.GetStats().m_DrawCalls++;
		g_Renderer.GetStats().m_OverlayTris += 2;
	}

	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);

	glEnable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);
	glDisable(GL_TEXTURE_2D);
#endif
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

	if (!m_Line->m_SimContext)
	{
		debug_warn(L"[OverlayRenderer] No SimContext set for textured overlay line, cannot render (no terrain data)");
		return;
	}

	const CTerrain& terrain = m_Line->m_SimContext->GetTerrain();
	CmpPtr<ICmpWaterManager> cmpWaterManager(*m_Line->m_SimContext, SYSTEM_ENTITY);

	float v = 0.f;
	std::vector<SVertex> vertices;
	std::vector<u16> indices;

	size_t n = m_Line->m_Coords.size() / 2; // number of line points
	bool closed = m_Line->m_Closed;

	ENSURE(n >= 2); // minimum needed to avoid errors (also minimum value to make sense, can't draw a line between 1 point)

	// In each iteration, p1 is the position of vertex i, p0 is i-1, p2 is i+1.
	// To avoid slightly expensive terrain computations we cycle these around and
	// recompute p2 at the end of each iteration.

	CVector3D p0;
	CVector3D p1(m_Line->m_Coords[0], 0, m_Line->m_Coords[1]);
	CVector3D p2(m_Line->m_Coords[(1 % n)*2], 0, m_Line->m_Coords[(1 % n)*2+1]);

	if (closed)
		// grab the ending point so as to close the loop
		p0 = CVector3D(m_Line->m_Coords[(n-1)*2], 0, m_Line->m_Coords[(n-1)*2+1]);
	else
		// we don't want to loop around and use the direction towards the other end of the line, so create an artificial p0 that 
		// extends the p2 -> p1 direction, and use that point instead
		p0 = p1 + (p1 - p2);

	bool p1floating = false;
	bool p2floating = false;

	// Compute terrain heights, clamped to the water height (and remember whether
	// each point was floating on water, for normal computation later)

	// TODO: if we ever support more than one water level per map, recompute this per point
	float w = cmpWaterManager->GetExactWaterLevel(p0.X, p0.Z);

	p0.Y = terrain.GetExactGroundLevel(p0.X, p0.Z);
	if (p0.Y < w)
		p0.Y = w;

	p1.Y = terrain.GetExactGroundLevel(p1.X, p1.Z);
	if (p1.Y < w)
	{
		p1.Y = w;
		p1floating = true;
	}

	p2.Y = terrain.GetExactGroundLevel(p2.X, p2.Z);
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
			norm = terrain.CalcExactNormal(p1.X, p1.Z);

		CVector3D b = ((p1 - p0).Normalized() + (p2 - p1).Normalized()).Cross(norm);

		// Adjust bisector length to match the line thickness, along the line's width
		float l = b.Dot((p2 - p1).Normalized().Cross(norm));
		if (fabs(l) > 0.000001f) // avoid unlikely divide-by-zero
			b *= m_Line->m_Thickness / l;

		// Push vertices and indices in GL_TRIANGLES order
		// 
		// NOTE: in order for OpenGL to successfully render these, the winding order needs to be correct. Basically, it means
		// that every pair of triangles sharing a side must specify the vertices of that side in the opposite order from the 
		// other triangle.
		// (see http://www.opengl.org/resources/code/samples/sig99/advanced99/notes/node16.html for an illustration)
		// 
		// What the code below does is push the indices for a quad composed of two triangles in each iteration. The two triangles 
		// of each quad are indexed using the winding orders (BR, BL, TR) and (TR, BL, TR) (where BR is bottom-right of this
		// iteration's quad, TR top-right etc).
		SVertex vertex1(p1 + b + norm*OverlayRendererInternals::OVERLAY_VOFFSET, 0.f, v);
		SVertex vertex2(p1 - b + norm*OverlayRendererInternals::OVERLAY_VOFFSET, 1.f, v);
		vertices.push_back(vertex1);
		vertices.push_back(vertex2);

		u16 index1 = vertices.size() - 2; // index of vertex1 in this iteration (TR of this quad)
		u16 index2 = vertices.size() - 1; // index of the vertex2 in this iteration (TL of this quad)

		if (i == 0)
		{
			// initial two vertices to continue building triangles from (n must be >= 2 for this to work)
			indices.push_back(index1);
			indices.push_back(index2);
		}
		else 
		{
			u16 index1Prev = vertices.size() - 4; // index of the vertex1 in the previous iteration (BR of this quad)
			u16 index2Prev = vertices.size() - 3; // index of the vertex2 in the previous iteration (BL of this quad)
			ENSURE(index1Prev < vertices.size());
			ENSURE(index2Prev < vertices.size());
			// Add two corner points from last iteration and join with one of our own corners to create triangle 1
			// (don't need to do this if i == 1 because i == 0 are the first two ones, they don't need to be copied)
			if (i > 1)
			{
				indices.push_back(index1Prev);
				indices.push_back(index2Prev);
			}
			indices.push_back(index1); // complete triangle 1

			// create triangle 2, specifying the adjacent side's vertices in the opposite order from triangle 1
			indices.push_back(index1);
			indices.push_back(index2Prev);
			indices.push_back(index2);
		}

		// alternate V coordinate for debugging
		v = 1 - v;

		// cycle the p's and compute the new p2
		p0 = p1;
		p1 = p2;
		p1floating = p2floating;

		// if in closed mode, wrap around the coordinate array for p2 -- otherwise, extend linearly
		if (!closed && i == n-2)
			// next iteration is the last point of the line, so create an artificial p2 that extends the p0 -> p1 direction
			p2 = p1 + (p1 - p0);
		else
			p2 = CVector3D(m_Line->m_Coords[((i+2) % n)*2], 0, m_Line->m_Coords[((i+2) % n)*2+1]);

		p2.Y = terrain.GetExactGroundLevel(p2.X, p2.Z);
		if (p2.Y < w)
		{
			p2.Y = w;
			p2floating = true;
		}
		else
			p2floating = false;
	}

	if (closed)
	{
		// close the path
		indices.push_back(vertices.size()-2);
		indices.push_back(vertices.size()-1);
		indices.push_back(0);

		indices.push_back(0);
		indices.push_back(vertices.size()-1);
		indices.push_back(1);
	}
	else
	{
		// Create start and end caps. On either end, this is done by taking the centroid between the last and second-to-last pair of
		// vertices that was generated along the path (i.e. the vertex1's and vertex2's from above), taking a directional vector 
		// between them, and drawing the line cap in the plane given by the two butt-end corner points plus said vector.
		std::vector<u16> capIndices;
		std::vector<SVertex> capVertices;

		// create end cap
		CreateLineCap(
			// the order of these vertices is important here, swapping them produces caps at the wrong side
			vertices[vertices.size()-2].m_Position, // top-right vertex of last quad
			vertices[vertices.size()-1].m_Position, // top-left vertex of last quad
			// directional vector between centroids of last vertex pair and second-to-last vertex pair
			(Centroid(vertices[vertices.size()-2], vertices[vertices.size()-1]) - Centroid(vertices[vertices.size()-4], vertices[vertices.size()-3])).Normalized(),
			m_Line->m_EndCapType,
			capVertices,
			capIndices
		);

		for (unsigned i = 0; i < capIndices.size(); i++)
			capIndices[i] += vertices.size();

		vertices.insert(vertices.end(), capVertices.begin(), capVertices.end());
		indices.insert(indices.end(), capIndices.begin(), capIndices.end());

		capIndices.clear();
		capVertices.clear();

		// create start cap
		CreateLineCap(
			// the order of these vertices is important here, swapping them produces caps at the wrong side
			vertices[1].m_Position,
			vertices[0].m_Position,
			// directional vector between centroids of first vertex pair and second vertex pair
			(Centroid(vertices[1], vertices[0]) - Centroid(vertices[3], vertices[2])).Normalized(),
			m_Line->m_StartCapType,
			capVertices,
			capIndices
		);

		for (unsigned i = 0; i < capIndices.size(); i++)
			capIndices[i] += vertices.size();

		vertices.insert(vertices.end(), capVertices.begin(), capVertices.end());
		indices.insert(indices.end(), capIndices.begin(), capIndices.end());
	}

	ENSURE(indices.size() % 3 == 0); // GL_TRIANGLES indices, so must be multiple of 3

	m_VB = g_VBMan.Allocate(sizeof(SVertex), vertices.size(), GL_STATIC_DRAW, GL_ARRAY_BUFFER);
	if (m_VB)
	{
		// allocation might fail (e.g. due to too many vertices)
		m_VB->m_Owner->UpdateChunkVertices(m_VB, &vertices[0]); // copy data into VBO

		for (size_t k = 0; k < indices.size(); ++k)
			indices[k] += m_VB->m_Index;

		m_VBIndices = g_VBMan.Allocate(sizeof(u16), indices.size(), GL_STATIC_DRAW, GL_ELEMENT_ARRAY_BUFFER);
		if (m_VBIndices)
			m_VBIndices->m_Owner->UpdateChunkVertices(m_VBIndices, &indices[0]);
	}
	
}

void CTexturedLineRData::CreateLineCap(const CVector3D& corner1, const CVector3D& corner2, const CVector3D& lineDirectionNormal,
	                                   SOverlayTexturedLine::LineCapType endCapType, std::vector<SVertex>& verticesOut,
									   std::vector<u16>& indicesOut)
{
	if (endCapType == SOverlayTexturedLine::LINECAP_FLAT)
		return; // no action needed, this is the default

	// When not in closed mode, we've created artificial points for the start- and endpoints that extend the line in the
	// direction of the first and the last segment, respectively. Thus, we know both the start and endpoints have perpendicular
	// butt endings, i.e. the end corner vertices on either side of the line extend perpendicularly from the segment direction.
	// That is to say, when viewed from the top, we will have something like
	//                                                 .
	//  this:                     and not like this:  /|
	//         ----+                                 / |
	//             |                                /  .
	//             |                                  /
	//         ----+                                 /
	//

	int roundCapPoints = 8; // amount of points to sample along the semicircle for rounded caps (including corner points)
	float radius = m_Line->m_Thickness;

	CVector3D centerPoint = (corner1 + corner2) * 0.5f;
	SVertex centerVertex(centerPoint, 0.5f, 0.5f);
	u16 indexOffset = verticesOut.size(); // index offset in verticesOut from where we start adding our vertices

	switch (endCapType)
	{
	case SOverlayTexturedLine::LINECAP_SHARP:
		{
			roundCapPoints = 3; // creates only one point directly ahead
			radius *= 1.5f; // make it a bit sharper (note that we don't use the radius for the butt-end corner points so it should be ok)
			centerVertex.m_UVs[0] = 0.480f; // slight visual correction to make the texture match up better at the corner points
		}
		// fall-through
	case SOverlayTexturedLine::LINECAP_ROUND:
		{
			// Draw a rounded line cap in the 3D plane of the line specified by the two corner points and the normal vector of the 
			// line's direction. The terrain normal at the centroid between the two corner points is perpendicular to this plane.
			// The way this works is by taking a vector from the corner points' centroid to one of the corner points (which is then 
			// of radius length), and rotate it around the terrain normal vector in that centroid. This will rotate the vector in 
			// the line's plane, producing the desired rounded cap.

			// To please OpenGL's winding order, this angle needs to be negated depending on whether we start rotating from 
			// the (center -> corner1) or (center -> corner2) vector. For the (center -> corner2) vector, we apparently need to use 
			// the negated angle.
			float stepAngle = -(float)(M_PI/(roundCapPoints-1));

			// Push the vertices in triangle fan order (easy to generate GL_TRIANGLES indices for afterwards)
			// Note that we're manually adding the corner vertices instead of having them be generated by the rotating vector. 
			// This is because we want to support an overly large radius to make the sharp line ending look sharper.
			verticesOut.push_back(centerVertex); 
			verticesOut.push_back(SVertex(corner2, 0.f, 0.f));

			// Get the base vector that we will incrementally rotate in the cap plane to produce the radial sample points.
			// Normally corner2 - centerPoint would suffice for this since it is of radius length, but we want to support custom 
			// radii to support tuning the 'sharpness' of sharp end caps (see above)
			CVector3D rotationBaseVector = (corner2 - centerPoint).Normalized() * radius;
			// Calculate the normal vector of the plane in which we're going to be drawing the line cap. This is the vector that
			// is perpendicular to both baseVector and the 'lineDirectionNormal' vector indicating the direction of the line.
			// Note that we shouldn't use terrain->CalcExactNormal() here because if the line is being rendered on top of water,
			// then CalcExactNormal will return the normal vector of the terrain that's underwater (which can be quite funky).
			CVector3D capPlaneNormal = lineDirectionNormal.Cross(rotationBaseVector).Normalized();

			for (int i = 1; i < roundCapPoints - 1; ++i)
			{
				// Rotate the centerPoint -> corner vector by i*stepAngle radians around the cap plane normal at the center point.
				CQuaternion quatRotation;
				quatRotation.FromAxisAngle(capPlaneNormal, i * stepAngle);
				CVector3D worldPos3D = centerPoint + quatRotation.Rotate(rotationBaseVector);

				// Let v range from 0 to 1 as we move along the semi-circle, keep u fixed at 0 (i.e. curve the left vertical edge 
				// of the texture around the edge of the semicircle)
				float u = 0.f;
				float v = clamp((i/(float)(roundCapPoints-1)), 0.f, 1.f); // pos, u, v
				verticesOut.push_back(SVertex(worldPos3D, u, v));
			}

			// connect back to the other butt-end corner point to complete the semicircle 
			verticesOut.push_back(SVertex(corner1, 0.f, 1.f)); 

			// now push indices in GL_TRIANGLES order; vertices[indexOffset] is the center vertex, vertices[indexOffset + 1] is the 
			// first corner point, then a bunch of radial samples, and then at the end we have the other corner point again. So: 
			for (int i=1; i < roundCapPoints; ++i)
			{
				indicesOut.push_back(indexOffset); // center vertex 
				indicesOut.push_back(indexOffset + i); 
				indicesOut.push_back(indexOffset + i + 1); 
			}
		}
		break;

	case SOverlayTexturedLine::LINECAP_SQUARE:
		{
			// Extend the (corner1 -> corner2) vector along the direction normal and draw a square line ending consisting of 
			// three triangles (sort of like a triangle fan)
			// NOTE: The order in which the vertices are pushed out determines the visibility, as they
			// are rendered only one-sided; the wrong order of vertices will make the cap visible only from the bottom.
			verticesOut.push_back(centerVertex);
			verticesOut.push_back(SVertex(corner2, 0.f, 0.f));
			verticesOut.push_back(SVertex(corner2 + (lineDirectionNormal * (m_Line->m_Thickness)), 0.f, 0.33333f)); // extend butt corner point 2 along the normal vector 
			verticesOut.push_back(SVertex(corner1 + (lineDirectionNormal * (m_Line->m_Thickness)), 0.f, 0.66666f)); // extend butt corner point 1 along the normal vector 
			verticesOut.push_back(SVertex(corner1, 0.f, 1.0f)); // push butt corner point 1 

			for (int i=1; i < 4; ++i) 
			{ 
				indicesOut.push_back(indexOffset); // center point 
				indicesOut.push_back(indexOffset + i); 
				indicesOut.push_back(indexOffset + i + 1);
			} 
		}
		break;

	default:
		break;
	}

}
