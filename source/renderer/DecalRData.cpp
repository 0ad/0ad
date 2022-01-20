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

#include "DecalRData.h"

#include "graphics/Decal.h"
#include "graphics/Model.h"
#include "graphics/ShaderManager.h"
#include "graphics/Terrain.h"
#include "graphics/TextureManager.h"
#include "lib/allocators/DynamicArena.h"
#include "lib/allocators/STLAllocators.h"
#include "ps/CLogger.h"
#include "ps/CStrInternStatic.h"
#include "ps/Game.h"
#include "ps/Profile.h"
#include "renderer/Renderer.h"
#include "renderer/TerrainRenderer.h"
#include "simulation2/components/ICmpWaterManager.h"
#include "simulation2/Simulation2.h"

#include <algorithm>

// TODO: Currently each decal is a separate CDecalRData. We might want to use
// lots of decals for special effects like shadows, footprints, etc, in which
// case we should probably redesign this to batch them all together for more
// efficient rendering.

namespace
{

struct SDecalBatch
{
	CDecalRData* decal;
	CShaderTechniquePtr shaderTech;
	CVertexBuffer::VBChunk* vertices;
	CVertexBuffer::VBChunk* indices;
};

struct SDecalBatchComparator
{
	bool operator()(const SDecalBatch& lhs, const SDecalBatch& rhs) const
	{
		if (lhs.shaderTech != rhs.shaderTech)
			return lhs.shaderTech < rhs.shaderTech;
		if (lhs.vertices->m_Owner != rhs.vertices->m_Owner)
			return lhs.vertices->m_Owner < rhs.vertices->m_Owner;
		if (lhs.indices->m_Owner != rhs.indices->m_Owner)
			return lhs.indices->m_Owner < rhs.indices->m_Owner;
		return lhs.decal < rhs.decal;
	}
};

} // anonymous namespace

CDecalRData::CDecalRData(CModelDecal* decal, CSimulation2* simulation)
	: m_Decal(decal), m_Simulation(simulation)
{
	BuildVertexData();
}

CDecalRData::~CDecalRData() = default;

void CDecalRData::Update(CSimulation2* simulation)
{
	m_Simulation = simulation;
	if (m_UpdateFlags != 0)
	{
		BuildVertexData();
		m_UpdateFlags = 0;
	}
}

void CDecalRData::RenderDecals(
	Renderer::Backend::GL::CDeviceCommandContext* deviceCommandContext,
	const std::vector<CDecalRData*>& decals, const CShaderDefines& context, ShadowMap* shadow)
{
	PROFILE3("render terrain decals");

	using Arena = Allocators::DynamicArena<256 * KiB>;

	Arena arena;

	using Batches = std::vector<SDecalBatch, ProxyAllocator<SDecalBatch, Arena>>;
	Batches batches((Batches::allocator_type(arena)));
	batches.reserve(decals.size());

	CShaderDefines contextDecal = context;
	contextDecal.Add(str_DECAL, str_1);

	for (CDecalRData* decal : decals)
	{
		CMaterial &material = decal->m_Decal->m_Decal.m_Material;

		if (material.GetShaderEffect().empty())
		{
			LOGERROR("Terrain renderer failed to load shader effect.\n");
			continue;
		}

		CShaderDefines defines = contextDecal;
		defines.SetMany(material.GetShaderDefines(0));
		CShaderTechniquePtr techBase = g_Renderer.GetShaderManager().LoadEffect(
			material.GetShaderEffect(), defines);
		if (!techBase)
		{
			LOGERROR("Terrain renderer failed to load shader effect (%s)\n",
					material.GetShaderEffect().string().c_str());
			continue;
		}

		if (material.GetSamplers().empty() || !decal->m_VBDecals || !decal->m_VBDecalsIndices)
			continue;

		SDecalBatch batch;
		batch.decal = decal;
		batch.shaderTech = techBase;
		batch.vertices = decal->m_VBDecals.Get();
		batch.indices = decal->m_VBDecalsIndices.Get();

		batches.emplace_back(std::move(batch));
	}

	if (batches.empty())
		return;

	std::sort(batches.begin(), batches.end(), SDecalBatchComparator());

	CVertexBuffer* lastIB = nullptr;
	for (auto itTechBegin = batches.begin(), itTechEnd = batches.begin(); itTechBegin != batches.end(); itTechBegin = itTechEnd)
	{
		while (itTechEnd != batches.end() && itTechBegin->shaderTech == itTechEnd->shaderTech)
			++itTechEnd;

		const CShaderTechniquePtr& techBase = itTechBegin->shaderTech;
		const int numPasses = techBase->GetNumPasses();

		for (int pass = 0; pass < numPasses; ++pass)
		{
			Renderer::Backend::GraphicsPipelineStateDesc pipelineStateDesc =
				techBase->GetGraphicsPipelineStateDesc(pass);
			pipelineStateDesc.blendState.enabled = true;
			pipelineStateDesc.blendState.srcColorBlendFactor = pipelineStateDesc.blendState.srcAlphaBlendFactor =
				Renderer::Backend::BlendFactor::SRC_ALPHA;
			pipelineStateDesc.blendState.dstColorBlendFactor = pipelineStateDesc.blendState.dstAlphaBlendFactor =
				Renderer::Backend::BlendFactor::ONE_MINUS_SRC_ALPHA;
			pipelineStateDesc.blendState.colorBlendOp = pipelineStateDesc.blendState.alphaBlendOp =
				Renderer::Backend::BlendOp::ADD;
			techBase->BeginPass(pass);
			deviceCommandContext->SetGraphicsPipelineState(pipelineStateDesc);

			const CShaderProgramPtr& shader = techBase->GetShader(pass);
			TerrainRenderer::PrepareShader(shader, shadow);

			CVertexBuffer* lastVB = nullptr;
			for (auto itDecal = itTechBegin; itDecal != itTechEnd; ++itDecal)
			{
				SDecalBatch& batch = *itDecal;
				CDecalRData* decal = batch.decal;
				CMaterial& material = decal->m_Decal->m_Decal.m_Material;

				const CMaterial::SamplersVector& samplers = material.GetSamplers();
				for (const CMaterial::TextureSampler& sampler : samplers)
					shader->BindTexture(sampler.Name, sampler.Sampler);

				material.GetStaticUniforms().BindUniforms(shader);

				// TODO: Need to handle floating decals correctly. In particular, we need
				// to render non-floating before water and floating after water (to get
				// the blending right), and we also need to apply the correct lighting in
				// each case, which doesn't really seem possible with the current
				// TerrainRenderer.
				// Also, need to mark the decals as dirty when water height changes.

				//	m_Decal->GetBounds().Render();

				shader->Uniform(str_shadingColor, decal->m_Decal->GetShadingColor());

				if (lastVB != batch.vertices->m_Owner)
				{
					lastVB = batch.vertices->m_Owner;
					const GLsizei stride = sizeof(SDecalVertex);
					SDecalVertex* base = (SDecalVertex*)batch.vertices->m_Owner->Bind();

					shader->VertexPointer(3, GL_FLOAT, stride, &base->m_Position[0]);
					shader->NormalPointer(GL_FLOAT, stride, &base->m_Normal[0]);
					shader->TexCoordPointer(GL_TEXTURE0, 2, GL_FLOAT, stride, &base->m_UV[0]);
				}

				shader->AssertPointersBound();

				if (lastIB != batch.indices->m_Owner)
				{
					lastIB = batch.indices->m_Owner;
					batch.indices->m_Owner->Bind();
				}

				u8* indexBase = nullptr;
				glDrawElements(GL_TRIANGLES, batch.indices->m_Count, GL_UNSIGNED_SHORT, indexBase + sizeof(u16) * (batch.indices->m_Index));

				// bump stats
				g_Renderer.m_Stats.m_DrawCalls++;
				g_Renderer.m_Stats.m_TerrainTris += batch.indices->m_Count / 3;
			}

			techBase->EndPass();
		}
	}

	CVertexBuffer::Unbind();
}

void CDecalRData::BuildVertexData()
{
	PROFILE("decal build");

	const SDecal& decal = m_Decal->m_Decal;

	// TODO: Currently this constructs an axis-aligned bounding rectangle around
	// the decal. It would be more efficient for rendering if we excluded tiles
	// that are outside the (non-axis-aligned) decal rectangle.

	ssize_t i0, j0, i1, j1;
	m_Decal->CalcVertexExtents(i0, j0, i1, j1);
	// Currently CalcVertexExtents might return empty rectangle, that means
	// we can't render it.
	if (i1 <= i0 && j1 <= j0)
	{
		// We have nothing to render.
		m_VBDecals.Reset();
		m_VBDecalsIndices.Reset();
		return;
	}

	CmpPtr<ICmpWaterManager> cmpWaterManager(*m_Simulation, SYSTEM_ENTITY);

	std::vector<SDecalVertex> vertices((i1 - i0 + 1) * (j1 - j0 + 1));

	for (ssize_t j = j0, idx = 0; j <= j1; ++j)
	{
		for (ssize_t i = i0; i <= i1; ++i, ++idx)
		{
			SDecalVertex& vertex = vertices[idx];
			m_Decal->m_Terrain->CalcPosition(i, j, vertex.m_Position);

			if (decal.m_Floating && cmpWaterManager)
			{
				vertex.m_Position.Y = std::max(
					vertex.m_Position.Y,
					cmpWaterManager->GetExactWaterLevel(vertex.m_Position.X, vertex.m_Position.Z));
			}

			m_Decal->m_Terrain->CalcNormal(i, j, vertex.m_Normal);

			// Map from world space back into decal texture space.
			CVector3D inv = m_Decal->GetInvTransform().Transform(vertex.m_Position);
			vertex.m_UV.X = 0.5f + (inv.X - decal.m_OffsetX) / decal.m_SizeX;
			// Flip V to match our texture convention.
			vertex.m_UV.Y = 0.5f - (inv.Z - decal.m_OffsetZ) / decal.m_SizeZ;
		}
	}

	if (!m_VBDecals || m_VBDecals->m_Count != vertices.size())
		m_VBDecals = g_VBMan.AllocateChunk(sizeof(SDecalVertex), vertices.size(), GL_STATIC_DRAW, GL_ARRAY_BUFFER);
	m_VBDecals->m_Owner->UpdateChunkVertices(m_VBDecals.Get(), vertices.data());

	std::vector<u16> indices((i1 - i0) * (j1 - j0) * 6);

	const ssize_t w = i1 - i0 + 1;
	auto itIdx = indices.begin();
	const size_t base = m_VBDecals->m_Index;
	for (ssize_t dj = 0; dj < j1 - j0; ++dj)
	{
		for (ssize_t di = 0; di < i1 - i0; ++di)
		{
			const bool dir = m_Decal->m_Terrain->GetTriangulationDir(i0 + di, j0 + dj);
			if (dir)
			{
				*itIdx++ = u16(((dj + 0) * w + (di + 0)) + base);
				*itIdx++ = u16(((dj + 0) * w + (di + 1)) + base);
				*itIdx++ = u16(((dj + 1) * w + (di + 0)) + base);

				*itIdx++ = u16(((dj + 0) * w + (di + 1)) + base);
				*itIdx++ = u16(((dj + 1) * w + (di + 1)) + base);
				*itIdx++ = u16(((dj + 1) * w + (di + 0)) + base);
			}
			else
			{
				*itIdx++ = u16(((dj + 0) * w + (di + 0)) + base);
				*itIdx++ = u16(((dj + 0) * w + (di + 1)) + base);
				*itIdx++ = u16(((dj + 1) * w + (di + 1)) + base);

				*itIdx++ = u16(((dj + 1) * w + (di + 1)) + base);
				*itIdx++ = u16(((dj + 1) * w + (di + 0)) + base);
				*itIdx++ = u16(((dj + 0) * w + (di + 0)) + base);
			}
		}
	}

	// Construct vertex buffer.
	if (!m_VBDecalsIndices || m_VBDecalsIndices->m_Count != indices.size())
		m_VBDecalsIndices = g_VBMan.AllocateChunk(sizeof(u16), indices.size(), GL_STATIC_DRAW, GL_ELEMENT_ARRAY_BUFFER);
	m_VBDecalsIndices->m_Owner->UpdateChunkVertices(m_VBDecalsIndices.Get(), indices.data());
}
