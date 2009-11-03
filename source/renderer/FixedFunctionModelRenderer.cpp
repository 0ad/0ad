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

/*
 * Implementation of FixedFunctionModelRenderer
 */

#include "precompiled.h"

#include "lib/ogl.h"
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


#define LOG_CATEGORY L"graphics"


///////////////////////////////////////////////////////////////////////////////////////////////
// FixedFunctionModelRenderer implementation

struct FFModelDef : public CModelDefRPrivate
{
	/// Indices are the same for all models, so share them
	u16* m_Indices;

	/// Static per-CModelDef vertex array
	VertexArray m_Array;

	/// UV coordinates are stored in the static array
	VertexArray::Attribute m_UV;

	FFModelDef(const CModelDefPtr& mdef);
	~FFModelDef() { delete[] m_Indices; }
};


FFModelDef::FFModelDef(const CModelDefPtr& mdef)
	: m_Array(false)
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

	m_Indices = new u16[mdef->GetNumFaces()*3];
	ModelRenderer::BuildIndices(mdef, m_Indices);
}


struct FFModel
{
	/// Dynamic per-CModel vertex array
	VertexArray m_Array;

	/// Position and lighting are recalculated on CPU every frame
	VertexArray::Attribute m_Position;
	VertexArray::Attribute m_Color;

	FFModel() : m_Array(true) { }
};


struct FixedFunctionModelRendererInternals
{
	/// Transformed vertex normals - required for recalculating lighting on skinned models
	std::vector<CVector3D> normals;

	/// Previously prepared modeldef
	FFModelDef* ffmodeldef;

	/// If true, primary color will only contain the diffuse term
	bool colorIsDiffuseOnly;
};


// Construction and Destruction
FixedFunctionModelRenderer::FixedFunctionModelRenderer(bool colorIsDiffuseOnly)
{
	m = new FixedFunctionModelRendererInternals;
	m->ffmodeldef = 0;
	m->colorIsDiffuseOnly = colorIsDiffuseOnly;
}

FixedFunctionModelRenderer::~FixedFunctionModelRenderer()
{
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

	ffmodel->m_Position.type = GL_FLOAT;
	ffmodel->m_Position.elems = 3;
	ffmodel->m_Array.AddAttribute(&ffmodel->m_Position);

	ffmodel->m_Color.type = GL_UNSIGNED_BYTE;
	ffmodel->m_Color.elems = 4;
	ffmodel->m_Array.AddAttribute(&ffmodel->m_Color);

	ffmodel->m_Array.SetNumVertices(mdef->GetNumVertices());
	ffmodel->m_Array.Layout();

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
		if (m->normals.size() < numVertices)
			m->normals.resize(numVertices);

		VertexArrayIterator<CVector3D> Position = ffmodel->m_Position.GetIterator<CVector3D>();
		VertexArrayIterator<CVector3D> Normal = VertexArrayIterator<CVector3D>((char*)&m->normals[0], sizeof(CVector3D));

		ModelRenderer::BuildPositionAndNormals(model, Position, Normal);

		VertexArrayIterator<SColor4ub> Color = ffmodel->m_Color.GetIterator<SColor4ub>();

		ModelRenderer::BuildColor4ub(model, Normal, Color, m->colorIsDiffuseOnly);

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
void FixedFunctionModelRenderer::BeginPass(int streamflags, const CMatrix3D* texturematrix)
{
	debug_assert(streamflags == (streamflags & (STREAM_POS|STREAM_UV0|STREAM_COLOR|STREAM_TEXGENTOUV1)));

	glEnableClientState(GL_VERTEX_ARRAY);

	if (streamflags & STREAM_UV0) glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	if (streamflags & STREAM_COLOR) glEnableClientState(GL_COLOR_ARRAY);
	if (streamflags & STREAM_TEXGENTOUV1)
	{
		pglActiveTextureARB(GL_TEXTURE1);
		pglClientActiveTextureARB(GL_TEXTURE1);

		glEnableClientState(GL_TEXTURE_COORD_ARRAY);

		glMatrixMode(GL_TEXTURE);
		glLoadMatrixf(&texturematrix->_11);
		glMatrixMode(GL_MODELVIEW);

		pglActiveTextureARB(GL_TEXTURE0);
		pglClientActiveTextureARB(GL_TEXTURE0);
	}
}


// Cleanup one rendering pass
void FixedFunctionModelRenderer::EndPass(int streamflags)
{
	if (streamflags & STREAM_UV0) glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	if (streamflags & STREAM_COLOR) glDisableClientState(GL_COLOR_ARRAY);
	if (streamflags & STREAM_TEXGENTOUV1)
	{
		pglActiveTextureARB(GL_TEXTURE1);
		pglClientActiveTextureARB(GL_TEXTURE1);

		glDisableClientState(GL_TEXTURE_COORD_ARRAY);

		glMatrixMode(GL_TEXTURE);
		glLoadIdentity();
		glMatrixMode(GL_MODELVIEW);

		pglActiveTextureARB(GL_TEXTURE0);
		pglClientActiveTextureARB(GL_TEXTURE0);
	}

	glDisableClientState(GL_VERTEX_ARRAY);
}


// Prepare UV coordinates for this modeldef
void FixedFunctionModelRenderer::PrepareModelDef(int streamflags, const CModelDefPtr& def)
{
	m->ffmodeldef = (FFModelDef*)def->GetRenderData(m);

	debug_assert(m->ffmodeldef);

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

	glVertexPointer(3, GL_FLOAT, stride, base + ffmodel->m_Position.offset);
	if (streamflags & STREAM_COLOR)
		glColorPointer(3, ffmodel->m_Color.type, stride, base + ffmodel->m_Color.offset);
	if (streamflags & STREAM_TEXGENTOUV1)
	{
		pglClientActiveTextureARB(GL_TEXTURE1);
		pglActiveTextureARB(GL_TEXTURE1);
		glTexCoordPointer(3, GL_FLOAT, stride, base + ffmodel->m_Position.offset);
		pglClientActiveTextureARB(GL_TEXTURE0);
		pglActiveTextureARB(GL_TEXTURE0);
	}

	// render the lot
	size_t numFaces = mdldef->GetNumFaces();

	if (!g_Renderer.m_SkipSubmit) {
		pglDrawRangeElementsEXT(GL_TRIANGLES, 0, (GLuint)mdldef->GetNumVertices(),
					   (GLsizei)numFaces*3, GL_UNSIGNED_SHORT, m->ffmodeldef->m_Indices);
	}

	// bump stats
	g_Renderer.m_Stats.m_DrawCalls++;
	g_Renderer.m_Stats.m_ModelTris += numFaces;
}
