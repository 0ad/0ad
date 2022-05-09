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
#include "renderer/InstancingModelRenderer.h"

#include "graphics/Color.h"
#include "graphics/LightEnv.h"
#include "graphics/Model.h"
#include "graphics/ModelDef.h"
#include "maths/Vector3D.h"
#include "maths/Vector4D.h"
#include "ps/CLogger.h"
#include "ps/CStrInternStatic.h"
#include "renderer/Renderer.h"
#include "renderer/RenderModifiers.h"
#include "renderer/VertexArray.h"
#include "third_party/mikktspace/weldmesh.h"


struct IModelDef : public CModelDefRPrivate
{
	/// Static per-CModel vertex array
	VertexArray m_Array;

	/// Position and normals are static
	VertexArray::Attribute m_Position;
	VertexArray::Attribute m_Normal;
	VertexArray::Attribute m_Tangent;
	VertexArray::Attribute m_BlendJoints; // valid iff gpuSkinning == true
	VertexArray::Attribute m_BlendWeights; // valid iff gpuSkinning == true

	/// The number of UVs is determined by the model
	std::vector<VertexArray::Attribute> m_UVs;

	/// Indices are the same for all models, so share them
	VertexIndexArray m_IndexArray;

	IModelDef(const CModelDefPtr& mdef, bool gpuSkinning, bool calculateTangents);
};


IModelDef::IModelDef(const CModelDefPtr& mdef, bool gpuSkinning, bool calculateTangents)
	: m_IndexArray(false), m_Array(Renderer::Backend::IBuffer::Type::VERTEX, false)
{
	size_t numVertices = mdef->GetNumVertices();

	m_Position.format = Renderer::Backend::Format::R32G32B32_SFLOAT;
	m_Array.AddAttribute(&m_Position);

	m_Normal.format = Renderer::Backend::Format::R32G32B32_SFLOAT;
	m_Array.AddAttribute(&m_Normal);

	m_UVs.resize(mdef->GetNumUVsPerVertex());
	for (size_t i = 0; i < mdef->GetNumUVsPerVertex(); i++)
	{
		m_UVs[i].format = Renderer::Backend::Format::R32G32_SFLOAT;
		m_Array.AddAttribute(&m_UVs[i]);
	}

	if (gpuSkinning)
	{
		// We can't use a lot of bones because it costs uniform memory. Recommended
		// number of bones per model is 32.
		// Add 1 to NumBones because of the special 'root' bone.
		if (mdef->GetNumBones() + 1 > 64)
			LOGERROR("Model '%s' has too many bones %zu/64", mdef->GetName().string8().c_str(), mdef->GetNumBones() + 1);
		ENSURE(mdef->GetNumBones() + 1 <= 64);

		m_BlendJoints.format = Renderer::Backend::Format::R8G8B8A8_UINT;
		m_Array.AddAttribute(&m_BlendJoints);

		m_BlendWeights.format = Renderer::Backend::Format::R8G8B8A8_UNORM;
		m_Array.AddAttribute(&m_BlendWeights);
	}

	if (calculateTangents)
	{
		// Generate tangents for the geometry:-

		m_Tangent.format = Renderer::Backend::Format::R32G32B32A32_SFLOAT;
		m_Array.AddAttribute(&m_Tangent);

		// floats per vertex; position + normal + tangent + UV*sets [+ GPUskinning]
		int numVertexAttrs = 3 + 3 + 4 + 2 * mdef->GetNumUVsPerVertex();
		if (gpuSkinning)
		{
			numVertexAttrs += 8;
		}

		// the tangent generation can increase the number of vertices temporarily
		// so reserve a bit more memory to avoid reallocations in GenTangents (in most cases)
		std::vector<float> newVertices;
		newVertices.reserve(numVertexAttrs * numVertices * 2);

		// Generate the tangents
		ModelRenderer::GenTangents(mdef, newVertices, gpuSkinning);

		// how many vertices do we have after generating tangents?
		int newNumVert = newVertices.size() / numVertexAttrs;

		std::vector<int> remapTable(newNumVert);
		std::vector<float> vertexDataOut(newNumVert * numVertexAttrs);

		// re-weld the mesh to remove duplicated vertices
		int numVertices2 = WeldMesh(&remapTable[0], &vertexDataOut[0],
					&newVertices[0], newNumVert, numVertexAttrs);

		// Copy the model data to graphics memory:-

		m_Array.SetNumberOfVertices(numVertices2);
		m_Array.Layout();

		VertexArrayIterator<CVector3D> Position = m_Position.GetIterator<CVector3D>();
		VertexArrayIterator<CVector3D> Normal = m_Normal.GetIterator<CVector3D>();
		VertexArrayIterator<CVector4D> Tangent = m_Tangent.GetIterator<CVector4D>();

		VertexArrayIterator<u8[4]> BlendJoints;
		VertexArrayIterator<u8[4]> BlendWeights;
		if (gpuSkinning)
		{
			BlendJoints = m_BlendJoints.GetIterator<u8[4]>();
			BlendWeights = m_BlendWeights.GetIterator<u8[4]>();
		}

		// copy everything into the vertex array
		for (int i = 0; i < numVertices2; i++)
		{
			int q = numVertexAttrs * i;

			Position[i] = CVector3D(vertexDataOut[q + 0], vertexDataOut[q + 1], vertexDataOut[q + 2]);
			q += 3;

			Normal[i] = CVector3D(vertexDataOut[q + 0], vertexDataOut[q + 1], vertexDataOut[q + 2]);
			q += 3;

			Tangent[i] = CVector4D(vertexDataOut[q + 0], vertexDataOut[q + 1], vertexDataOut[q + 2],
					vertexDataOut[q + 3]);
			q += 4;

			if (gpuSkinning)
			{
				for (size_t j = 0; j < 4; ++j)
				{
					BlendJoints[i][j] = (u8)vertexDataOut[q + 0 + 2 * j];
					BlendWeights[i][j] = (u8)vertexDataOut[q + 1 + 2 * j];
				}
				q += 8;
			}

			for (size_t j = 0; j < mdef->GetNumUVsPerVertex(); j++)
			{
				VertexArrayIterator<float[2]> UVit = m_UVs[j].GetIterator<float[2]>();
				UVit[i][0] = vertexDataOut[q + 0 + 2 * j];
				UVit[i][1] = vertexDataOut[q + 1 + 2 * j];
			}
		}

		// upload vertex data
		m_Array.Upload();
		m_Array.FreeBackingStore();

		m_IndexArray.SetNumberOfVertices(mdef->GetNumFaces() * 3);
		m_IndexArray.Layout();

		VertexArrayIterator<u16> Indices = m_IndexArray.GetIterator();

		size_t idxidx = 0;

		// reindex geometry and upload index
		for (size_t j = 0; j < mdef->GetNumFaces(); ++j)
		{
			Indices[idxidx++] = remapTable[j * 3 + 0];
			Indices[idxidx++] = remapTable[j * 3 + 1];
			Indices[idxidx++] = remapTable[j * 3 + 2];
		}

		m_IndexArray.Upload();
		m_IndexArray.FreeBackingStore();
	}
	else
	{
		// Upload model without calculating tangents:-

		m_Array.SetNumberOfVertices(numVertices);
		m_Array.Layout();

		VertexArrayIterator<CVector3D> Position = m_Position.GetIterator<CVector3D>();
		VertexArrayIterator<CVector3D> Normal = m_Normal.GetIterator<CVector3D>();

		ModelRenderer::CopyPositionAndNormals(mdef, Position, Normal);

		for (size_t i = 0; i < mdef->GetNumUVsPerVertex(); i++)
		{
			VertexArrayIterator<float[2]> UVit = m_UVs[i].GetIterator<float[2]>();
			ModelRenderer::BuildUV(mdef, UVit, i);
		}

		if (gpuSkinning)
		{
			VertexArrayIterator<u8[4]> BlendJoints = m_BlendJoints.GetIterator<u8[4]>();
			VertexArrayIterator<u8[4]> BlendWeights = m_BlendWeights.GetIterator<u8[4]>();
			for (size_t i = 0; i < numVertices; ++i)
			{
				const SModelVertex& vtx = mdef->GetVertices()[i];
				for (size_t j = 0; j < 4; ++j)
				{
					BlendJoints[i][j] = vtx.m_Blend.m_Bone[j];
					BlendWeights[i][j] = (u8)(255.f * vtx.m_Blend.m_Weight[j]);
				}
			}
		}

		m_Array.Upload();
		m_Array.FreeBackingStore();

		m_IndexArray.SetNumberOfVertices(mdef->GetNumFaces()*3);
		m_IndexArray.Layout();
		ModelRenderer::BuildIndices(mdef, m_IndexArray.GetIterator());
		m_IndexArray.Upload();
		m_IndexArray.FreeBackingStore();
	}
}


struct InstancingModelRendererInternals
{
	bool gpuSkinning;

	bool calculateTangents;

	/// Previously prepared modeldef
	IModelDef* imodeldef;

	/// Index base for imodeldef
	u8* imodeldefIndexBase;
};


// Construction and Destruction
InstancingModelRenderer::InstancingModelRenderer(bool gpuSkinning, bool calculateTangents)
{
	m = new InstancingModelRendererInternals;
	m->gpuSkinning = gpuSkinning;
	m->calculateTangents = calculateTangents;
	m->imodeldef = 0;
}

InstancingModelRenderer::~InstancingModelRenderer()
{
	delete m;
}


// Build modeldef data if necessary - we have no per-CModel data
CModelRData* InstancingModelRenderer::CreateModelData(const void* key, CModel* model)
{
	CModelDefPtr mdef = model->GetModelDef();
	IModelDef* imodeldef = (IModelDef*)mdef->GetRenderData(m);

	if (m->gpuSkinning)
 		ENSURE(model->IsSkinned());
	else
		ENSURE(!model->IsSkinned());

	if (!imodeldef)
	{
		imodeldef = new IModelDef(mdef, m->gpuSkinning, m->calculateTangents);
		mdef->SetRenderData(m, imodeldef);
	}

	return new CModelRData(key);
}


void InstancingModelRenderer::UpdateModelData(CModel* UNUSED(model), CModelRData* UNUSED(data), int UNUSED(updateflags))
{
	// We have no per-CModel data
}


// Setup one rendering pass.
void InstancingModelRenderer::BeginPass()
{
}

// Cleanup rendering pass.
void InstancingModelRenderer::EndPass(
	Renderer::Backend::IDeviceCommandContext* UNUSED(deviceCommandContext))
{
}

// Prepare UV coordinates for this modeldef
void InstancingModelRenderer::PrepareModelDef(
	Renderer::Backend::IDeviceCommandContext* deviceCommandContext,
	const CModelDef& def)
{
	m->imodeldef = (IModelDef*)def.GetRenderData(m);

	ENSURE(m->imodeldef);
	m->imodeldef->m_Array.UploadIfNeeded(deviceCommandContext);
	m->imodeldef->m_IndexArray.UploadIfNeeded(deviceCommandContext);

	deviceCommandContext->SetIndexBuffer(m->imodeldef->m_IndexArray.GetBuffer());

	const uint32_t stride = m->imodeldef->m_Array.GetStride();
	const uint32_t firstVertexOffset = m->imodeldef->m_Array.GetOffset() * stride;

	deviceCommandContext->SetVertexAttributeFormat(
		Renderer::Backend::VertexAttributeStream::POSITION,
		m->imodeldef->m_Position.format,
		firstVertexOffset + m->imodeldef->m_Position.offset, stride, 0);
	deviceCommandContext->SetVertexAttributeFormat(
		Renderer::Backend::VertexAttributeStream::NORMAL,
		m->imodeldef->m_Normal.format,
		firstVertexOffset + m->imodeldef->m_Normal.offset, stride, 0);

	constexpr size_t MAX_UV = 2;
	for (size_t uv = 0; uv < std::min(MAX_UV, def.GetNumUVsPerVertex()); ++uv)
	{
		const Renderer::Backend::VertexAttributeStream stream =
			static_cast<Renderer::Backend::VertexAttributeStream>(
				static_cast<int>(Renderer::Backend::VertexAttributeStream::UV0) + uv);
		deviceCommandContext->SetVertexAttributeFormat(
			stream, m->imodeldef->m_UVs[uv].format,
			firstVertexOffset + m->imodeldef->m_UVs[uv].offset, stride, 0);
	}

	// GPU skinning requires extra attributes to compute positions/normals.
	if (m->gpuSkinning)
	{
		deviceCommandContext->SetVertexAttributeFormat(
			Renderer::Backend::VertexAttributeStream::UV2,
			m->imodeldef->m_BlendJoints.format,
			firstVertexOffset + m->imodeldef->m_BlendJoints.offset, stride, 0);
		deviceCommandContext->SetVertexAttributeFormat(
			Renderer::Backend::VertexAttributeStream::UV3,
			m->imodeldef->m_BlendWeights.format,
			firstVertexOffset + m->imodeldef->m_BlendWeights.offset, stride, 0);
	}

	if (m->calculateTangents)
	{
		deviceCommandContext->SetVertexAttributeFormat(
			Renderer::Backend::VertexAttributeStream::UV4,
			m->imodeldef->m_Tangent.format,
			firstVertexOffset + m->imodeldef->m_Tangent.offset, stride, 0);
	}

	deviceCommandContext->SetVertexBuffer(0, m->imodeldef->m_Array.GetBuffer());
}


// Render one model
void InstancingModelRenderer::RenderModel(
	Renderer::Backend::IDeviceCommandContext* deviceCommandContext,
	Renderer::Backend::IShaderProgram* shader, CModel* model, CModelRData* UNUSED(data))
{
	const CModelDefPtr& mdldef = model->GetModelDef();

	if (m->gpuSkinning)
	{
		// Bind matrices for current animation state.
		// Add 1 to NumBones because of the special 'root' bone.
		deviceCommandContext->SetUniform(
			shader->GetBindingSlot(str_skinBlendMatrices),
			PS::span<const float>(
				model->GetAnimatedBoneMatrices()[0]._data,
				model->GetAnimatedBoneMatrices()[0].AsFloatArray().size() * (mdldef->GetNumBones() + 1)));
	}

	// Render the lot.
	const size_t numberOfFaces = mdldef->GetNumFaces();

	deviceCommandContext->DrawIndexedInRange(
		m->imodeldef->m_IndexArray.GetOffset(), numberOfFaces * 3, 0, m->imodeldef->m_Array.GetNumberOfVertices() - 1);

	// Bump stats.
	g_Renderer.m_Stats.m_DrawCalls++;
	g_Renderer.m_Stats.m_ModelTris += numberOfFaces;
}
