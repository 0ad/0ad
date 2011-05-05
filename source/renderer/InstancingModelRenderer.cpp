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
 * Implementation of InstancingModelRenderer
 */

#include "precompiled.h"

#include "lib/ogl.h"
#include "maths/Vector3D.h"
#include "maths/Vector4D.h"

#include "ps/CLogger.h"

#include "graphics/Color.h"
#include "graphics/LightEnv.h"
#include "graphics/Model.h"
#include "graphics/ModelDef.h"

#include "renderer/InstancingModelRenderer.h"
#include "renderer/Renderer.h"
#include "renderer/RenderModifiers.h"
#include "renderer/VertexArray.h"


///////////////////////////////////////////////////////////////////////////////////////////////
// InstancingModelRenderer implementation

struct IModelDef : public CModelDefRPrivate
{
	/// Static per-CModel vertex array
	VertexArray m_Array;

	/// Position, normals and UV are all static
	VertexArray::Attribute m_Position;
	VertexArray::Attribute m_Normal;
	VertexArray::Attribute m_UV;

	/// Indices are the same for all models, so share them
	VertexIndexArray m_IndexArray;


	IModelDef(const CModelDefPtr& mdef);
};


IModelDef::IModelDef(const CModelDefPtr& mdef)
	: m_IndexArray(GL_STATIC_DRAW), m_Array(GL_STATIC_DRAW)
{
	size_t numVertices = mdef->GetNumVertices();

	m_Position.type = GL_FLOAT;
	m_Position.elems = 3;
	m_Array.AddAttribute(&m_Position);

	m_Normal.type = GL_FLOAT;
	m_Normal.elems = 3;
	m_Array.AddAttribute(&m_Normal);

	m_UV.type = GL_FLOAT;
	m_UV.elems = 2;
	m_Array.AddAttribute(&m_UV);

	m_Array.SetNumVertices(numVertices);
	m_Array.Layout();

	VertexArrayIterator<CVector3D> Position = m_Position.GetIterator<CVector3D>();
	VertexArrayIterator<CVector3D> Normal = m_Normal.GetIterator<CVector3D>();
	VertexArrayIterator<float[2]> UVit = m_UV.GetIterator<float[2]>();

	ModelRenderer::CopyPositionAndNormals(mdef, Position, Normal);
	ModelRenderer::BuildUV(mdef, UVit);

	m_Array.Upload();
	m_Array.FreeBackingStore();

	m_IndexArray.SetNumVertices(mdef->GetNumFaces()*3);
	m_IndexArray.Layout();
	ModelRenderer::BuildIndices(mdef, m_IndexArray.GetIterator());
	m_IndexArray.Upload();
	m_IndexArray.FreeBackingStore();
}


struct InstancingModelRendererInternals
{
	/// Currently used RenderModifier
	RenderModifierPtr modifier;

	/// Previously prepared modeldef
	IModelDef* imodeldef;

	/// Index base for imodeldef
	u8* imodeldefIndexBase;
};


// Construction and Destruction
InstancingModelRenderer::InstancingModelRenderer()
{
	m = new InstancingModelRendererInternals;
	m->imodeldef = 0;
}

InstancingModelRenderer::~InstancingModelRenderer()
{
	delete m;
}


// Build modeldef data if necessary - we have no per-CModel data
void* InstancingModelRenderer::CreateModelData(CModel* model)
{
	CModelDefPtr mdef = model->GetModelDef();
	IModelDef* imodeldef = (IModelDef*)mdef->GetRenderData(m);

	ENSURE(!model->IsSkinned());

	if (!imodeldef)
	{
		imodeldef = new IModelDef(mdef);
		mdef->SetRenderData(m, imodeldef);
	}

	return NULL;
}


void InstancingModelRenderer::UpdateModelData(CModel* UNUSED(model), void* UNUSED(data), int UNUSED(updateflags))
{
	// We have no per-CModel data
}


void InstancingModelRenderer::DestroyModelData(CModel* UNUSED(model), void* UNUSED(data))
{
	// We have no per-CModel data, and per-CModelDef data is deleted by the CModelDef
}


// Setup one rendering pass.
void InstancingModelRenderer::BeginPass(int streamflags)
{
	ENSURE(streamflags == (streamflags & (STREAM_POS|STREAM_NORMAL|STREAM_UV0)));

	if (streamflags & STREAM_POS)
		glEnableClientState(GL_VERTEX_ARRAY);

	if (streamflags & STREAM_NORMAL)
		glEnableClientState(GL_NORMAL_ARRAY);

	if (streamflags & STREAM_UV0)
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
}

// Cleanup rendering pass.
void InstancingModelRenderer::EndPass(int streamflags)
{
	if (streamflags & STREAM_POS)
		glDisableClientState(GL_VERTEX_ARRAY);

	if (streamflags & STREAM_NORMAL)
		glDisableClientState(GL_NORMAL_ARRAY);

	if (streamflags & STREAM_UV0)
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);

	CVertexBuffer::Unbind();
}


// Prepare UV coordinates for this modeldef
void InstancingModelRenderer::PrepareModelDef(int streamflags, const CModelDefPtr& def)
{
	m->imodeldef = (IModelDef*)def->GetRenderData(m);

	ENSURE(m->imodeldef);

	u8* base = m->imodeldef->m_Array.Bind();
	GLsizei stride = (GLsizei)m->imodeldef->m_Array.GetStride();

	m->imodeldefIndexBase = m->imodeldef->m_IndexArray.Bind();

	if (streamflags & STREAM_POS)
		glVertexPointer(3, GL_FLOAT, stride, base + m->imodeldef->m_Position.offset);

	if (streamflags & STREAM_NORMAL)
		glNormalPointer(GL_FLOAT, stride, base + m->imodeldef->m_Normal.offset);

	if (streamflags & STREAM_UV0)
		glTexCoordPointer(2, GL_FLOAT, stride, base + m->imodeldef->m_UV.offset);
}


// Render one model
void InstancingModelRenderer::RenderModel(int UNUSED(streamflags), CModel* model, void* UNUSED(data))
{
	CModelDefPtr mdldef = model->GetModelDef();

	// render the lot
	size_t numFaces = mdldef->GetNumFaces();

	if (!g_Renderer.m_SkipSubmit)
	{
		pglDrawRangeElementsEXT(GL_TRIANGLES, 0, (GLuint)mdldef->GetNumVertices()-1,
				(GLsizei)numFaces*3, GL_UNSIGNED_SHORT, m->imodeldefIndexBase);
	}

	// bump stats
	g_Renderer.m_Stats.m_DrawCalls++;
	g_Renderer.m_Stats.m_ModelTris += numFaces;

}
