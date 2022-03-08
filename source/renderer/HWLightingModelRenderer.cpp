/* Copyright (C) 2022 Wildfire Games.
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

#include "renderer/HWLightingModelRenderer.h"

#include "graphics/Color.h"
#include "graphics/LightEnv.h"
#include "graphics/Model.h"
#include "graphics/ModelDef.h"
#include "graphics/ShaderProgram.h"
#include "lib/bits.h"
#include "lib/ogl.h"
#include "lib/sysdep/rtl.h"
#include "maths/Vector3D.h"
#include "renderer/Renderer.h"
#include "renderer/RenderModifiers.h"
#include "renderer/VertexArray.h"


struct ShaderModelDef : public CModelDefRPrivate
{
	/// Indices are the same for all models, so share them
	VertexIndexArray m_IndexArray;

	/// Static per-CModelDef vertex array
	VertexArray m_Array;

	/// The number of UVs is determined by the model
	std::vector<VertexArray::Attribute> m_UVs;

	ShaderModelDef(const CModelDefPtr& mdef);
};


ShaderModelDef::ShaderModelDef(const CModelDefPtr& mdef)
	: m_IndexArray(false),
	m_Array(Renderer::Backend::GL::CBuffer::Type::VERTEX, false)
{
	size_t numVertices = mdef->GetNumVertices();

	m_UVs.resize(mdef->GetNumUVsPerVertex());
	for (size_t i = 0; i < mdef->GetNumUVsPerVertex(); ++i)
	{
		m_UVs[i].format = Renderer::Backend::Format::R32G32_SFLOAT;
		m_Array.AddAttribute(&m_UVs[i]);
	}

	m_Array.SetNumberOfVertices(numVertices);
	m_Array.Layout();

	for (size_t i = 0; i < mdef->GetNumUVsPerVertex(); ++i)
	{
		VertexArrayIterator<float[2]> UVit = m_UVs[i].GetIterator<float[2]>();
		ModelRenderer::BuildUV(mdef, UVit, i);
	}

	m_Array.Upload();
	m_Array.FreeBackingStore();

	m_IndexArray.SetNumberOfVertices(mdef->GetNumFaces()*3);
	m_IndexArray.Layout();
	ModelRenderer::BuildIndices(mdef, m_IndexArray.GetIterator());
	m_IndexArray.Upload();
	m_IndexArray.FreeBackingStore();
}


struct ShaderModel : public CModelRData
{
	/// Dynamic per-CModel vertex array
	VertexArray m_Array;

	/// Position and normals/lighting are recalculated on CPU every frame
	VertexArray::Attribute m_Position;
	VertexArray::Attribute m_Normal;

	ShaderModel(const void* key)
		: CModelRData(key),
		m_Array(Renderer::Backend::GL::CBuffer::Type::VERTEX, true)
	{}
};


struct ShaderModelVertexRenderer::ShaderModelRendererInternals
{
	/// Previously prepared modeldef
	ShaderModelDef* shadermodeldef;
};


// Construction and Destruction
ShaderModelVertexRenderer::ShaderModelVertexRenderer()
{
	m = new ShaderModelRendererInternals;
	m->shadermodeldef = nullptr;
}

ShaderModelVertexRenderer::~ShaderModelVertexRenderer()
{
	delete m;
}


// Build model data (and modeldef data if necessary)
CModelRData* ShaderModelVertexRenderer::CreateModelData(const void* key, CModel* model)
{
	CModelDefPtr mdef = model->GetModelDef();
	ShaderModelDef* shadermodeldef = (ShaderModelDef*)mdef->GetRenderData(m);

	if (!shadermodeldef)
	{
		shadermodeldef = new ShaderModelDef(mdef);
		mdef->SetRenderData(m, shadermodeldef);
	}

	// Build the per-model data
	ShaderModel* shadermodel = new ShaderModel(key);

	// Positions and normals must be 16-byte aligned for SSE writes.

	shadermodel->m_Position.format = Renderer::Backend::Format::R32G32B32A32_SFLOAT;
	shadermodel->m_Array.AddAttribute(&shadermodel->m_Position);

	shadermodel->m_Normal.format = Renderer::Backend::Format::R32G32B32A32_SFLOAT;
	shadermodel->m_Array.AddAttribute(&shadermodel->m_Normal);

	shadermodel->m_Array.SetNumberOfVertices(mdef->GetNumVertices());
	shadermodel->m_Array.Layout();

	// Verify alignment
	ENSURE(shadermodel->m_Position.offset % 16 == 0);
	ENSURE(shadermodel->m_Normal.offset % 16 == 0);
	ENSURE(shadermodel->m_Array.GetStride() % 16 == 0);

	return shadermodel;
}


// Fill in and upload dynamic vertex array
void ShaderModelVertexRenderer::UpdateModelData(CModel* model, CModelRData* data, int updateflags)
{
	ShaderModel* shadermodel = static_cast<ShaderModel*>(data);

	if (updateflags & RENDERDATA_UPDATE_VERTICES)
	{
		// build vertices
		VertexArrayIterator<CVector3D> Position = shadermodel->m_Position.GetIterator<CVector3D>();
		VertexArrayIterator<CVector3D> Normal = shadermodel->m_Normal.GetIterator<CVector3D>();

		ModelRenderer::BuildPositionAndNormals(model, Position, Normal);

		// upload everything to vertex buffer
		shadermodel->m_Array.Upload();
	}

	shadermodel->m_Array.PrepareForRendering();
}


// Setup one rendering pass
void ShaderModelVertexRenderer::BeginPass(int streamflags)
{
	ENSURE(streamflags == (streamflags & (STREAM_POS | STREAM_UV0 | STREAM_UV1 | STREAM_NORMAL)));
}

// Cleanup one rendering pass
void ShaderModelVertexRenderer::EndPass(
	Renderer::Backend::GL::CDeviceCommandContext* deviceCommandContext, int UNUSED(streamflags))
{
	CVertexBuffer::Unbind(deviceCommandContext);
}


// Prepare UV coordinates for this modeldef
void ShaderModelVertexRenderer::PrepareModelDef(
	Renderer::Backend::GL::CDeviceCommandContext* deviceCommandContext,
	const CShaderProgramPtr& shader, int streamflags, const CModelDef& def)
{
	m->shadermodeldef = (ShaderModelDef*)def.GetRenderData(m);

	ENSURE(m->shadermodeldef);

	u8* base = m->shadermodeldef->m_Array.Bind(deviceCommandContext);
	GLsizei stride = (GLsizei)m->shadermodeldef->m_Array.GetStride();

	if (streamflags & STREAM_UV0)
	{
		shader->TexCoordPointer(
			GL_TEXTURE0, m->shadermodeldef->m_UVs[0].format, stride,
			base + m->shadermodeldef->m_UVs[0].offset);
	}

	if ((streamflags & STREAM_UV1) && def.GetNumUVsPerVertex() >= 2)
	{
		shader->TexCoordPointer(
			GL_TEXTURE1, m->shadermodeldef->m_UVs[1].format, stride,
			base + m->shadermodeldef->m_UVs[1].offset);
	}
}


// Render one model
void ShaderModelVertexRenderer::RenderModel(
	Renderer::Backend::GL::CDeviceCommandContext* deviceCommandContext,
	const CShaderProgramPtr& shader, int streamflags, CModel* model, CModelRData* data)
{
	const CModelDefPtr& mdldef = model->GetModelDef();
	ShaderModel* shadermodel = static_cast<ShaderModel*>(data);

	u8* base = shadermodel->m_Array.Bind(deviceCommandContext);
	GLsizei stride = (GLsizei)shadermodel->m_Array.GetStride();

	m->shadermodeldef->m_IndexArray.UploadIfNeeded(deviceCommandContext);
	deviceCommandContext->SetIndexBuffer(m->shadermodeldef->m_IndexArray.GetBuffer());

	if (streamflags & STREAM_POS)
	{
		shader->VertexPointer(
			Renderer::Backend::Format::R32G32B32_SFLOAT, stride,
			base + shadermodel->m_Position.offset);
	}

	if (streamflags & STREAM_NORMAL)
	{
		shader->NormalPointer(
			Renderer::Backend::Format::R32G32B32_SFLOAT, stride,
			base + shadermodel->m_Normal.offset);
	}

	shader->AssertPointersBound();

	// Render the lot.
	const size_t numberOfFaces = mdldef->GetNumFaces();

	deviceCommandContext->DrawIndexedInRange(
		m->shadermodeldef->m_IndexArray.GetOffset(), numberOfFaces * 3, 0, mdldef->GetNumVertices() - 1);

	// Bump stats.
	g_Renderer.m_Stats.m_DrawCalls++;
	g_Renderer.m_Stats.m_ModelTris += numberOfFaces;
}

