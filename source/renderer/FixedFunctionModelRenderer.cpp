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

/*
 * Implementation of FixedFunctionModelRenderer
 */

#include "precompiled.h"

#include "lib/bits.h"
#include "lib/ogl.h"
#include "lib/sysdep/rtl.h"
#include "maths/Vector3D.h"
#include "maths/Vector4D.h"

#include "ps/CLogger.h"

#include "graphics/SColor.h"
#include "graphics/Model.h"
#include "graphics/ModelDef.h"

#include "renderer/FixedFunctionModelRenderer.h"
#include "renderer/Renderer.h"
#include "renderer/RenderModifiers.h"
#include "renderer/VertexArray.h"


///////////////////////////////////////////////////////////////////////////////////////////////
// FixedFunctionModelRenderer implementation

struct FFModelDef : public CModelDefRPrivate
{
	/// Indices are the same for all models, so share them
	VertexIndexArray m_IndexArray;

	/// Static per-CModelDef vertex array
	VertexArray m_Array;

	/// UV coordinates are stored in the static array
	VertexArray::Attribute m_UV;

	FFModelDef(const CModelDefPtr& mdef);
};


FFModelDef::FFModelDef(const CModelDefPtr& mdef)
	: m_IndexArray(GL_STATIC_DRAW), m_Array(GL_STATIC_DRAW)
{
	size_t numVertices = mdef->GetNumVertices();

	m_UV.type = GL_FLOAT;
	m_UV.elems = 2;
	m_Array.AddAttribute(&m_UV);

	m_Array.SetNumVertices(numVertices);
	m_Array.Layout();

	VertexArrayIterator<float[2]> UVit = m_UV.GetIterator<float[2]>();

	ModelRenderer::BuildUV(mdef, UVit);

	m_Array.Upload();
	m_Array.FreeBackingStore();

	m_IndexArray.SetNumVertices(mdef->GetNumFaces()*3);
	m_IndexArray.Layout();
	ModelRenderer::BuildIndices(mdef, m_IndexArray.GetIterator());
	m_IndexArray.Upload();
	m_IndexArray.FreeBackingStore();
}


struct FFModel
{
	/// Dynamic per-CModel vertex array
	VertexArray m_Array;

	/// Position and lighting are recalculated on CPU every frame
	VertexArray::Attribute m_Position;
	VertexArray::Attribute m_Color;

	FFModel() : m_Array(GL_DYNAMIC_DRAW) { }
};


struct FixedFunctionModelRendererInternals
{
	/**
	 * Scratch space for normal vector calculation.
	 * Space is reserved so we don't have to do frequent reallocations.
	 * Allocated with rtl_AllocateAligned(normalsNumVertices*16, 16) for SSE writes.
	 */
	char* normals;
	size_t normalsNumVertices;

	/// Previously prepared modeldef
	FFModelDef* ffmodeldef;
};


// Construction and Destruction
FixedFunctionModelRenderer::FixedFunctionModelRenderer()
{
	m = new FixedFunctionModelRendererInternals;
	m->ffmodeldef = 0;
	m->normals = 0;
	m->normalsNumVertices = 0;
}

FixedFunctionModelRenderer::~FixedFunctionModelRenderer()
{
	rtl_FreeAligned(m->normals);

	delete m;
}


// Build model data (and modeldef data if necessary)
void* FixedFunctionModelRenderer::CreateModelData(CModel* model)
{
	CModelDefPtr mdef = model->GetModelDef();
	FFModelDef* ffmodeldef = (FFModelDef*)mdef->GetRenderData(m);

	if (!ffmodeldef)
	{
		ffmodeldef = new FFModelDef(mdef);
		mdef->SetRenderData(m, ffmodeldef);
	}

	// Build the per-model data
	FFModel* ffmodel = new FFModel;

	// Positions must be 16-byte aligned for SSE writes.
	// We can pack the color after the position; it will be corrupted by
	// BuildPositionAndNormals, but that's okay since we'll recompute the
	// colors afterwards.

	ffmodel->m_Color.type = GL_UNSIGNED_BYTE;
	ffmodel->m_Color.elems = 4;
	ffmodel->m_Array.AddAttribute(&ffmodel->m_Color);

	ffmodel->m_Position.type = GL_FLOAT;
	ffmodel->m_Position.elems = 3;
	ffmodel->m_Array.AddAttribute(&ffmodel->m_Position);

	ffmodel->m_Array.SetNumVertices(mdef->GetNumVertices());
	ffmodel->m_Array.Layout();

	// Verify alignment
	ENSURE(ffmodel->m_Position.offset % 16 == 0);
	ENSURE(ffmodel->m_Array.GetStride() % 16 == 0);

	return ffmodel;
}


// Fill in and upload dynamic vertex array
void FixedFunctionModelRenderer::UpdateModelData(CModel* model, void* data, int updateflags)
{
	FFModel* ffmodel = (FFModel*)data;

	if (updateflags & (RENDERDATA_UPDATE_VERTICES|RENDERDATA_UPDATE_COLOR))
	{
		CModelDefPtr mdef = model->GetModelDef();
		size_t numVertices = mdef->GetNumVertices();

		// build vertices

		// allocate working space for computing normals
		if (numVertices > m->normalsNumVertices)
		{
			rtl_FreeAligned(m->normals);

			size_t newSize = round_up_to_pow2(numVertices);
			m->normals = (char*)rtl_AllocateAligned(newSize*16, 16);
			m->normalsNumVertices = newSize;
		}

		VertexArrayIterator<CVector3D> Position = ffmodel->m_Position.GetIterator<CVector3D>();
		VertexArrayIterator<CVector3D> Normal = VertexArrayIterator<CVector3D>(m->normals, 16);

		ModelRenderer::BuildPositionAndNormals(model, Position, Normal);

		VertexArrayIterator<SColor4ub> Color = ffmodel->m_Color.GetIterator<SColor4ub>();

		ModelRenderer::BuildColor4ub(model, Normal, Color);

		// upload everything to vertex buffer
		ffmodel->m_Array.Upload();
	}
}


// Cleanup per-model data.
// Note that per-CModelDef data is deleted by the CModelDef itself.
void FixedFunctionModelRenderer::DestroyModelData(CModel* UNUSED(model), void* data)
{
	FFModel* ffmodel = (FFModel*)data;

	delete ffmodel;
}


// Setup one rendering pass
void FixedFunctionModelRenderer::BeginPass(int streamflags)
{
	ENSURE(streamflags == (streamflags & (STREAM_POS|STREAM_UV0|STREAM_COLOR)));

	glEnableClientState(GL_VERTEX_ARRAY);

	if (streamflags & STREAM_UV0) glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	if (streamflags & STREAM_COLOR) glEnableClientState(GL_COLOR_ARRAY);
}


// Cleanup one rendering pass
void FixedFunctionModelRenderer::EndPass(int streamflags)
{
	if (streamflags & STREAM_UV0) glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	if (streamflags & STREAM_COLOR) glDisableClientState(GL_COLOR_ARRAY);

	glDisableClientState(GL_VERTEX_ARRAY);

	CVertexBuffer::Unbind();
}


// Prepare UV coordinates for this modeldef
void FixedFunctionModelRenderer::PrepareModelDef(int streamflags, const CModelDefPtr& def)
{
	m->ffmodeldef = (FFModelDef*)def->GetRenderData(m);

	ENSURE(m->ffmodeldef);

	if (streamflags & STREAM_UV0)
	{
		u8* base = m->ffmodeldef->m_Array.Bind();
		GLsizei stride = (GLsizei)m->ffmodeldef->m_Array.GetStride();

		glTexCoordPointer(2, GL_FLOAT, stride, base + m->ffmodeldef->m_UV.offset);
	}
}


// Render one model
void FixedFunctionModelRenderer::RenderModel(int streamflags, CModel* model, void* data)
{
	CModelDefPtr mdldef = model->GetModelDef();
	FFModel* ffmodel = (FFModel*)data;

	u8* base = ffmodel->m_Array.Bind();
	GLsizei stride = (GLsizei)ffmodel->m_Array.GetStride();

	u8* indexBase = m->ffmodeldef->m_IndexArray.Bind();

	glVertexPointer(3, GL_FLOAT, stride, base + ffmodel->m_Position.offset);
	if (streamflags & STREAM_COLOR)
		glColorPointer(3, ffmodel->m_Color.type, stride, base + ffmodel->m_Color.offset);

	// render the lot
	size_t numFaces = mdldef->GetNumFaces();

	if (!g_Renderer.m_SkipSubmit) {
		pglDrawRangeElementsEXT(GL_TRIANGLES, 0, (GLuint)mdldef->GetNumVertices()-1,
					   (GLsizei)numFaces*3, GL_UNSIGNED_SHORT, indexBase);
	}

	// bump stats
	g_Renderer.m_Stats.m_DrawCalls++;
	g_Renderer.m_Stats.m_ModelTris += numFaces;
}
