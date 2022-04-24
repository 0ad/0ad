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

#include "OverlayRenderer.h"

#include "graphics/Camera.h"
#include "graphics/LOSTexture.h"
#include "graphics/Overlay.h"
#include "graphics/ShaderManager.h"
#include "graphics/Terrain.h"
#include "graphics/TextureManager.h"
#include "lib/hash.h"
#include "lib/ogl.h"
#include "maths/MathUtil.h"
#include "maths/Quaternion.h"
#include "ps/CStrInternStatic.h"
#include "ps/Game.h"
#include "ps/Profile.h"
#include "renderer/DebugRenderer.h"
#include "renderer/Renderer.h"
#include "renderer/SceneRenderer.h"
#include "renderer/TexturedLineRData.h"
#include "renderer/VertexArray.h"
#include "renderer/VertexBuffer.h"
#include "renderer/VertexBufferManager.h"
#include "simulation2/components/ICmpWaterManager.h"
#include "simulation2/Simulation2.h"
#include "simulation2/system/SimContext.h"

#include <unordered_map>

namespace
{

CShaderTechniquePtr GetOverlayLineShaderTechnique(const CShaderDefines& defines)
{
	return g_Renderer.GetShaderManager().LoadEffect(str_overlay_line, defines);
}

} // anonymous namespace

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

struct QuadBatchHash
{
	std::size_t operator()(const QuadBatchKey& d) const
	{
		size_t seed = 0;
		hash_combine(seed, d.m_Texture);
		hash_combine(seed, d.m_TextureMask);
		return seed;
	}
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
	using QuadBatchMap = std::unordered_map<QuadBatchKey, QuadBatchData, QuadBatchHash>;

	OverlayRendererInternals();
	~OverlayRendererInternals(){ }

	std::vector<SOverlayLine*> lines;
	std::vector<SOverlayTexturedLine*> texlines;
	std::vector<SOverlaySprite*> sprites;
	std::vector<SOverlayQuad*> quads;
	std::vector<SOverlaySphere*> spheres;

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

	// Geometry for a unit sphere
	std::vector<float> sphereVertexes;
	std::vector<u16> sphereIndexes;
	void GenerateSphere();

	/// Performs one-time setup. Called from CRenderer::Open, after graphics capabilities have
	/// been detected. Note that no VBOs must be created before this is called, since the shader
	/// path and graphics capabilities are not guaranteed to be stable before this point.
	void Initialize();
};

const float OverlayRenderer::OVERLAY_VOFFSET = 0.2f;

OverlayRendererInternals::OverlayRendererInternals()
	: quadVertices(Renderer::Backend::GL::CBuffer::Type::VERTEX, true),
	quadIndices(false)
{
	quadAttributePos.format = Renderer::Backend::Format::R32G32B32_SFLOAT;
	quadVertices.AddAttribute(&quadAttributePos);

	quadAttributeColor.format = Renderer::Backend::Format::R8G8B8A8_UNORM;
	quadVertices.AddAttribute(&quadAttributeColor);

	quadAttributeUV.format = Renderer::Backend::Format::R16G16_SINT;
	quadVertices.AddAttribute(&quadAttributeUV);

	// Note that we're reusing the textured overlay line shader for the quad overlay rendering. This
	// is because their code is almost identical; the only difference is that for the quad overlays
	// we want to use a vertex color stream as opposed to an objectColor uniform. To this end, the
	// shader has been set up to switch between the two behaviours based on the USE_OBJECTCOLOR define.
	defsOverlayLineNormal.Add(str_USE_OBJECTCOLOR, str_1);
	defsOverlayLineAlwaysVisible.Add(str_USE_OBJECTCOLOR, str_1);
	defsOverlayLineAlwaysVisible.Add(str_IGNORE_LOS, str_1);
}

void OverlayRendererInternals::Initialize()
{
	// Perform any initialization after graphics capabilities have been detected. Notably,
	// only at this point can we safely allocate VBOs (in contrast to e.g. in the constructor),
	// because their creation depends on the shader path, which is not reliably set before this point.

	quadVertices.SetNumberOfVertices(MAX_QUAD_OVERLAYS * 4);
	quadVertices.Layout(); // allocate backing store

	quadIndices.SetNumberOfVertices(MAX_QUAD_OVERLAYS * 6);
	quadIndices.Layout(); // allocate backing store

	// Since the quads in the vertex array are independent and always consist of exactly 4 vertices per quad, the
	// indices are always the same; we can therefore fill in all the indices once and pretty much forget about
	// them. We then also no longer need its backing store, since we never change any indices afterwards.
	VertexArrayIterator<u16> index = quadIndices.GetIterator();
	for (u16 i = 0; i < static_cast<u16>(MAX_QUAD_OVERLAYS); ++i)
	{
		*index++ = i * 4 + 0;
		*index++ = i * 4 + 1;
		*index++ = i * 4 + 2;
		*index++ = i * 4 + 2;
		*index++ = i * 4 + 3;
		*index++ = i * 4 + 0;
	}
	quadIndices.Upload();
	quadIndices.FreeBackingStore();
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
	m->lines.push_back(line);
}

void OverlayRenderer::Submit(SOverlayTexturedLine* line)
{
	// Simplify the rest of the code by guaranteeing non-empty lines
	if (line->m_Coords.empty())
		return;

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

void OverlayRenderer::Submit(SOverlaySphere* overlay)
{
	m->spheres.push_back(overlay);
}

void OverlayRenderer::EndFrame()
{
	m->lines.clear();
	m->texlines.clear();
	m->sprites.clear();
	m->quads.clear();
	m->spheres.clear();

	// this should leave the capacity unchanged, which is okay since it
	// won't be very large or very variable

	// Empty the batch rendering data structures, but keep their key mappings around for the next frames
	for (OverlayRendererInternals::QuadBatchMap::iterator it = m->quadBatchMap.begin(); it != m->quadBatchMap.end(); ++it)
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
	// color/sprite/etc for more efficient rendering

	for (size_t i = 0; i < m->texlines.size(); ++i)
	{
		SOverlayTexturedLine* line = m->texlines[i];
		if (!line->m_RenderData)
		{
			line->m_RenderData = std::make_shared<CTexturedLineRData>();
			line->m_RenderData->Update(*line);
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

	const CVector3D vOffset(0, OverlayRenderer::OVERLAY_VOFFSET, 0);

	// Write quad overlay vertices/indices to VA backing store
	VertexArrayIterator<CVector3D> vertexPos = m->quadAttributePos.GetIterator<CVector3D>();
	VertexArrayIterator<SColor4ub> vertexColor = m->quadAttributeColor.GetIterator<SColor4ub>();
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

			const SColor4ub quadColor = quad->m_Color.AsSColor4ub();

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

	m->quadVertices.PrepareForRendering();
}

void OverlayRenderer::RenderOverlaysBeforeWater(
	Renderer::Backend::GL::CDeviceCommandContext* deviceCommandContext)
{
	PROFILE3_GPU("overlays (before)");
	GPU_SCOPED_LABEL(deviceCommandContext, "Render overlays before water");

	for (SOverlayLine* line : m->lines)
	{
		if (line->m_Coords.empty())
			continue;

		g_Renderer.GetDebugRenderer().DrawLine(line->m_Coords, line->m_Color, static_cast<float>(line->m_Thickness));
	}
}

void OverlayRenderer::RenderOverlaysAfterWater(
	Renderer::Backend::GL::CDeviceCommandContext* deviceCommandContext)
{
	PROFILE3_GPU("overlays (after)");
	GPU_SCOPED_LABEL(deviceCommandContext, "Render overlays after water");

	RenderTexturedOverlayLines(deviceCommandContext);
	RenderQuadOverlays(deviceCommandContext);
	RenderSphereOverlays(deviceCommandContext);
}

void OverlayRenderer::RenderTexturedOverlayLines(Renderer::Backend::GL::CDeviceCommandContext* deviceCommandContext)
{
	if (m->texlines.empty())
		return;

	CLOSTexture& los = g_Renderer.GetSceneRenderer().GetScene().GetLOSTexture();

	// ----------------------------------------------------------------------------------------

	CShaderTechniquePtr shaderTechTexLineNormal = GetOverlayLineShaderTechnique(m->defsOverlayLineNormal);
	if (shaderTechTexLineNormal)
	{
		Renderer::Backend::GraphicsPipelineStateDesc pipelineStateDesc =
			shaderTechTexLineNormal->GetGraphicsPipelineStateDesc();
		pipelineStateDesc.depthStencilState.depthWriteEnabled = false;
		pipelineStateDesc.blendState.enabled = true;
		pipelineStateDesc.blendState.srcColorBlendFactor = pipelineStateDesc.blendState.srcAlphaBlendFactor =
			Renderer::Backend::BlendFactor::SRC_ALPHA;
		pipelineStateDesc.blendState.dstColorBlendFactor = pipelineStateDesc.blendState.dstAlphaBlendFactor =
			Renderer::Backend::BlendFactor::ONE_MINUS_SRC_ALPHA;
		pipelineStateDesc.blendState.colorBlendOp = pipelineStateDesc.blendState.alphaBlendOp =
			Renderer::Backend::BlendOp::ADD;
		if (g_Renderer.GetSceneRenderer().GetOverlayRenderMode() == WIREFRAME)
			pipelineStateDesc.rasterizationState.polygonMode = Renderer::Backend::PolygonMode::LINE;
		deviceCommandContext->SetGraphicsPipelineState(pipelineStateDesc);
		deviceCommandContext->BeginPass();

		Renderer::Backend::GL::CShaderProgram* shaderTexLineNormal = shaderTechTexLineNormal->GetShader();

		shaderTexLineNormal->BindTexture(str_losTex, los.GetTexture());
		shaderTexLineNormal->Uniform(str_losTransform, los.GetTextureMatrix()[0], los.GetTextureMatrix()[12], 0.f, 0.f);

		shaderTexLineNormal->Uniform(str_transform, g_Renderer.GetSceneRenderer().GetViewCamera().GetViewProjection());

		// batch render only the non-always-visible overlay lines using the normal shader
		RenderTexturedOverlayLines(deviceCommandContext, shaderTexLineNormal, false);

		deviceCommandContext->EndPass();
	}

	// ----------------------------------------------------------------------------------------

	CShaderTechniquePtr shaderTechTexLineAlwaysVisible = GetOverlayLineShaderTechnique(m->defsOverlayLineAlwaysVisible);
	if (shaderTechTexLineAlwaysVisible)
	{
		Renderer::Backend::GraphicsPipelineStateDesc pipelineStateDesc =
			shaderTechTexLineAlwaysVisible->GetGraphicsPipelineStateDesc();
		pipelineStateDesc.depthStencilState.depthWriteEnabled = false;
		pipelineStateDesc.blendState.enabled = true;
		pipelineStateDesc.blendState.srcColorBlendFactor = pipelineStateDesc.blendState.srcAlphaBlendFactor =
			Renderer::Backend::BlendFactor::SRC_ALPHA;
		pipelineStateDesc.blendState.dstColorBlendFactor = pipelineStateDesc.blendState.dstAlphaBlendFactor =
			Renderer::Backend::BlendFactor::ONE_MINUS_SRC_ALPHA;
		pipelineStateDesc.blendState.colorBlendOp = pipelineStateDesc.blendState.alphaBlendOp =
			Renderer::Backend::BlendOp::ADD;
		if (g_Renderer.GetSceneRenderer().GetOverlayRenderMode() == WIREFRAME)
			pipelineStateDesc.rasterizationState.polygonMode = Renderer::Backend::PolygonMode::LINE;
		deviceCommandContext->SetGraphicsPipelineState(pipelineStateDesc);
		deviceCommandContext->BeginPass();

		Renderer::Backend::GL::CShaderProgram* shaderTexLineAlwaysVisible = shaderTechTexLineAlwaysVisible->GetShader();

		// TODO: losTex and losTransform are unused in the always visible shader; see if these can be safely omitted
		shaderTexLineAlwaysVisible->BindTexture(str_losTex, los.GetTexture());
		shaderTexLineAlwaysVisible->Uniform(str_losTransform, los.GetTextureMatrix()[0], los.GetTextureMatrix()[12], 0.f, 0.f);

		shaderTexLineAlwaysVisible->Uniform(str_transform, g_Renderer.GetSceneRenderer().GetViewCamera().GetViewProjection());

		// batch render only the always-visible overlay lines using the LoS-ignored shader
		RenderTexturedOverlayLines(deviceCommandContext, shaderTexLineAlwaysVisible, true);

		deviceCommandContext->EndPass();
	}

	// ----------------------------------------------------------------------------------------

	// TODO: the shaders should probably be responsible for unbinding their textures
	deviceCommandContext->BindTexture(1, GL_TEXTURE_2D, 0);
	deviceCommandContext->BindTexture(0, GL_TEXTURE_2D, 0);
}

void OverlayRenderer::RenderTexturedOverlayLines(
	Renderer::Backend::GL::CDeviceCommandContext* deviceCommandContext,
	Renderer::Backend::GL::CShaderProgram* shader, bool alwaysVisible)
{
	for (size_t i = 0; i < m->texlines.size(); ++i)
	{
		SOverlayTexturedLine* line = m->texlines[i];

		// render only those lines matching the requested alwaysVisible status
		if (!line->m_RenderData || line->m_AlwaysVisible != alwaysVisible)
			continue;

		ENSURE(line->m_RenderData);
		line->m_RenderData->Render(deviceCommandContext, *line, shader);
	}
}

void OverlayRenderer::RenderQuadOverlays(
	Renderer::Backend::GL::CDeviceCommandContext* deviceCommandContext)
{
	if (m->quadBatchMap.empty())
		return;

	CShaderTechniquePtr shaderTech = GetOverlayLineShaderTechnique(m->defsQuadOverlay);

	if (!shaderTech)
		return;

	Renderer::Backend::GraphicsPipelineStateDesc pipelineStateDesc =
		shaderTech->GetGraphicsPipelineStateDesc();
	pipelineStateDesc.depthStencilState.depthWriteEnabled = false;
	pipelineStateDesc.blendState.enabled = true;
	pipelineStateDesc.blendState.srcColorBlendFactor = pipelineStateDesc.blendState.srcAlphaBlendFactor =
		Renderer::Backend::BlendFactor::SRC_ALPHA;
	pipelineStateDesc.blendState.dstColorBlendFactor = pipelineStateDesc.blendState.dstAlphaBlendFactor =
		Renderer::Backend::BlendFactor::ONE_MINUS_SRC_ALPHA;
	pipelineStateDesc.blendState.colorBlendOp = pipelineStateDesc.blendState.alphaBlendOp =
		Renderer::Backend::BlendOp::ADD;
	if (g_Renderer.GetSceneRenderer().GetOverlayRenderMode() == WIREFRAME)
		pipelineStateDesc.rasterizationState.polygonMode = Renderer::Backend::PolygonMode::LINE;
	deviceCommandContext->SetGraphicsPipelineState(pipelineStateDesc);
	deviceCommandContext->BeginPass();

	Renderer::Backend::GL::CShaderProgram* shader = shaderTech->GetShader();

	CLOSTexture& los = g_Renderer.GetSceneRenderer().GetScene().GetLOSTexture();

	shader->BindTexture(str_losTex, los.GetTexture());
	shader->Uniform(str_losTransform, los.GetTextureMatrix()[0], los.GetTextureMatrix()[12], 0.f, 0.f);

	shader->Uniform(str_transform, g_Renderer.GetSceneRenderer().GetViewCamera().GetViewProjection());

	m->quadIndices.UploadIfNeeded(deviceCommandContext);

	const uint32_t vertexStride = m->quadVertices.GetStride();
	const uint32_t firstVertexOffset = m->quadVertices.GetOffset() * vertexStride;

	for (OverlayRendererInternals::QuadBatchMap::iterator it = m->quadBatchMap.begin(); it != m->quadBatchMap.end(); ++it)
	{
		QuadBatchData& batchRenderData = it->second;
		const size_t batchNumQuads = batchRenderData.m_NumRenderQuads;

		if (batchNumQuads == 0)
			continue;

		const QuadBatchKey& maskPair = it->first;

		maskPair.m_Texture->UploadBackendTextureIfNeeded(deviceCommandContext);
		maskPair.m_TextureMask->UploadBackendTextureIfNeeded(deviceCommandContext);
		shader->BindTexture(str_baseTex, maskPair.m_Texture->GetBackendTexture());
		shader->BindTexture(str_maskTex, maskPair.m_TextureMask->GetBackendTexture());

		// TODO: move setting format out of the loop, we might want move the offset
		// to the index offset when it's supported.
		deviceCommandContext->SetVertexAttributeFormat(
			Renderer::Backend::VertexAttributeStream::POSITION,
			m->quadAttributePos.format, firstVertexOffset + m->quadAttributePos.offset, vertexStride, 0);
		deviceCommandContext->SetVertexAttributeFormat(
			Renderer::Backend::VertexAttributeStream::COLOR,
			m->quadAttributeColor.format, firstVertexOffset + m->quadAttributeColor.offset, vertexStride, 0);
		deviceCommandContext->SetVertexAttributeFormat(
			Renderer::Backend::VertexAttributeStream::UV0,
			m->quadAttributeUV.format, firstVertexOffset + m->quadAttributeUV.offset, vertexStride, 0);
		deviceCommandContext->SetVertexAttributeFormat(
			Renderer::Backend::VertexAttributeStream::UV1,
			m->quadAttributeUV.format, firstVertexOffset + m->quadAttributeUV.offset, vertexStride, 0);

		deviceCommandContext->SetVertexBuffer(0, m->quadVertices.GetBuffer());
		deviceCommandContext->SetIndexBuffer(m->quadIndices.GetBuffer());

		deviceCommandContext->DrawIndexed(m->quadIndices.GetOffset() + batchRenderData.m_IndicesBase, batchNumQuads * 6, 0);

		g_Renderer.GetStats().m_DrawCalls++;
		g_Renderer.GetStats().m_OverlayTris += batchNumQuads*2;
	}

	deviceCommandContext->EndPass();

	// TODO: the shader should probably be responsible for unbinding its textures
	deviceCommandContext->BindTexture(1, GL_TEXTURE_2D, 0);
	deviceCommandContext->BindTexture(0, GL_TEXTURE_2D, 0);
}

void OverlayRenderer::RenderForegroundOverlays(
	Renderer::Backend::GL::CDeviceCommandContext* deviceCommandContext,
	const CCamera& viewCamera)
{
	PROFILE3_GPU("overlays (fg)");

	const CVector3D right = -viewCamera.GetOrientation().GetLeft();
	const CVector3D up = viewCamera.GetOrientation().GetUp();

	CShaderTechniquePtr tech = g_Renderer.GetShaderManager().LoadEffect(str_foreground_overlay);
	Renderer::Backend::GraphicsPipelineStateDesc pipelineStateDesc =
		tech->GetGraphicsPipelineStateDesc();
	pipelineStateDesc.depthStencilState.depthTestEnabled = false;
	pipelineStateDesc.blendState.enabled = true;
	pipelineStateDesc.blendState.srcColorBlendFactor = pipelineStateDesc.blendState.srcAlphaBlendFactor =
		Renderer::Backend::BlendFactor::SRC_ALPHA;
	pipelineStateDesc.blendState.dstColorBlendFactor = pipelineStateDesc.blendState.dstAlphaBlendFactor =
		Renderer::Backend::BlendFactor::ONE_MINUS_SRC_ALPHA;
	pipelineStateDesc.blendState.colorBlendOp = pipelineStateDesc.blendState.alphaBlendOp =
		Renderer::Backend::BlendOp::ADD;
	if (g_Renderer.GetSceneRenderer().GetOverlayRenderMode() == WIREFRAME)
		pipelineStateDesc.rasterizationState.polygonMode = Renderer::Backend::PolygonMode::LINE;
	deviceCommandContext->SetGraphicsPipelineState(pipelineStateDesc);
	deviceCommandContext->BeginPass();

	Renderer::Backend::GL::CShaderProgram* shader = tech->GetShader();

	shader->Uniform(str_transform, g_Renderer.GetSceneRenderer().GetViewCamera().GetViewProjection());

	const CVector2D uvs[6] =
	{
		{0.0f, 1.0f},
		{1.0f, 1.0f},
		{1.0f, 0.0f},
		{0.0f, 1.0f},
		{1.0f, 0.0f},
		{0.0f, 0.0f},
	};

	deviceCommandContext->SetVertexAttributeFormat(
		Renderer::Backend::VertexAttributeStream::POSITION,
		Renderer::Backend::Format::R32G32B32_SFLOAT, 0, 0, 0);
	deviceCommandContext->SetVertexAttributeFormat(
		Renderer::Backend::VertexAttributeStream::UV0,
		Renderer::Backend::Format::R32G32_SFLOAT, 0, 0, 1);

	deviceCommandContext->SetVertexBufferData(1, &uvs[0]);

	for (size_t i = 0; i < m->sprites.size(); ++i)
	{
		SOverlaySprite* sprite = m->sprites[i];
		if (!i || sprite->m_Texture != m->sprites[i - 1]->m_Texture)
		{
			sprite->m_Texture->UploadBackendTextureIfNeeded(deviceCommandContext);
			shader->BindTexture(str_baseTex, sprite->m_Texture->GetBackendTexture());
		}

		shader->Uniform(str_colorMul, sprite->m_Color);

		const CVector3D position[6] =
		{
			sprite->m_Position + right*sprite->m_X0 + up*sprite->m_Y0,
			sprite->m_Position + right*sprite->m_X1 + up*sprite->m_Y0,
			sprite->m_Position + right*sprite->m_X1 + up*sprite->m_Y1,
			sprite->m_Position + right*sprite->m_X0 + up*sprite->m_Y0,
			sprite->m_Position + right*sprite->m_X1 + up*sprite->m_Y1,
			sprite->m_Position + right*sprite->m_X0 + up*sprite->m_Y1
		};

		deviceCommandContext->SetVertexBufferData(0, &position[0].X);

		deviceCommandContext->Draw(0, 6);

		g_Renderer.GetStats().m_DrawCalls++;
		g_Renderer.GetStats().m_OverlayTris += 2;
	}

	deviceCommandContext->EndPass();
}

static void TessellateSphereFace(const CVector3D& a, u16 ai,
								 const CVector3D& b, u16 bi,
								 const CVector3D& c, u16 ci,
								 std::vector<float>& vertexes, std::vector<u16>& indexes, int level)
{
	if (level == 0)
	{
		indexes.push_back(ai);
		indexes.push_back(bi);
		indexes.push_back(ci);
	}
	else
	{
		CVector3D d = (a + b).Normalized();
		CVector3D e = (b + c).Normalized();
		CVector3D f = (c + a).Normalized();
		int di = vertexes.size() / 3; vertexes.push_back(d.X); vertexes.push_back(d.Y); vertexes.push_back(d.Z);
		int ei = vertexes.size() / 3; vertexes.push_back(e.X); vertexes.push_back(e.Y); vertexes.push_back(e.Z);
		int fi = vertexes.size() / 3; vertexes.push_back(f.X); vertexes.push_back(f.Y); vertexes.push_back(f.Z);
		TessellateSphereFace(a,ai, d,di, f,fi, vertexes, indexes, level-1);
		TessellateSphereFace(d,di, b,bi, e,ei, vertexes, indexes, level-1);
		TessellateSphereFace(f,fi, e,ei, c,ci, vertexes, indexes, level-1);
		TessellateSphereFace(d,di, e,ei, f,fi, vertexes, indexes, level-1);
	}
}

static void TessellateSphere(std::vector<float>& vertexes, std::vector<u16>& indexes, int level)
{
	/* Start with a tetrahedron, then tessellate */
	float s = sqrtf(0.5f);
#define VERT(a,b,c) vertexes.push_back(a); vertexes.push_back(b); vertexes.push_back(c);
	VERT(-s,  0, -s);
	VERT( s,  0, -s);
	VERT( s,  0,  s);
	VERT(-s,  0,  s);
	VERT( 0, -1,  0);
	VERT( 0,  1,  0);
#define FACE(a,b,c) \
	TessellateSphereFace( \
		CVector3D(vertexes[a*3], vertexes[a*3+1], vertexes[a*3+2]), a, \
		CVector3D(vertexes[b*3], vertexes[b*3+1], vertexes[b*3+2]), b, \
		CVector3D(vertexes[c*3], vertexes[c*3+1], vertexes[c*3+2]), c, \
		vertexes, indexes, level);
	FACE(0,4,1);
	FACE(1,4,2);
	FACE(2,4,3);
	FACE(3,4,0);
	FACE(1,5,0);
	FACE(2,5,1);
	FACE(3,5,2);
	FACE(0,5,3);
#undef FACE
#undef VERT
}

void OverlayRendererInternals::GenerateSphere()
{
	if (sphereVertexes.empty())
		TessellateSphere(sphereVertexes, sphereIndexes, 3);
}

void OverlayRenderer::RenderSphereOverlays(
	Renderer::Backend::GL::CDeviceCommandContext* deviceCommandContext)
{
	PROFILE3_GPU("overlays (spheres)");

	if (m->spheres.empty())
		return;

	Renderer::Backend::GL::CShaderProgram* shader;
	CShaderTechniquePtr tech;

	tech = g_Renderer.GetShaderManager().LoadEffect(str_overlay_solid);
	Renderer::Backend::GraphicsPipelineStateDesc pipelineStateDesc =
		tech->GetGraphicsPipelineStateDesc();
	pipelineStateDesc.depthStencilState.depthWriteEnabled = false;
	pipelineStateDesc.blendState.enabled = true;
	pipelineStateDesc.blendState.srcColorBlendFactor = pipelineStateDesc.blendState.srcAlphaBlendFactor =
		Renderer::Backend::BlendFactor::SRC_ALPHA;
	pipelineStateDesc.blendState.dstColorBlendFactor = pipelineStateDesc.blendState.dstAlphaBlendFactor =
		Renderer::Backend::BlendFactor::ONE_MINUS_SRC_ALPHA;
	pipelineStateDesc.blendState.colorBlendOp = pipelineStateDesc.blendState.alphaBlendOp =
		Renderer::Backend::BlendOp::ADD;
	deviceCommandContext->SetGraphicsPipelineState(pipelineStateDesc);
	deviceCommandContext->BeginPass();

	shader = tech->GetShader();

	shader->Uniform(str_transform, g_Renderer.GetSceneRenderer().GetViewCamera().GetViewProjection());

	m->GenerateSphere();

	deviceCommandContext->SetVertexAttributeFormat(
		Renderer::Backend::VertexAttributeStream::POSITION,
		Renderer::Backend::Format::R32G32B32_SFLOAT, 0, 0, 0);

	deviceCommandContext->SetVertexBufferData(0, m->sphereVertexes.data());
	deviceCommandContext->SetIndexBufferData(m->sphereIndexes.data());

	for (size_t i = 0; i < m->spheres.size(); ++i)
	{
		SOverlaySphere* sphere = m->spheres[i];

		CMatrix3D transform;
		transform.SetIdentity();
		transform.Scale(sphere->m_Radius, sphere->m_Radius, sphere->m_Radius);
		transform.Translate(sphere->m_Center);

		shader->Uniform(str_instancingTransform, transform);

		shader->Uniform(str_color, sphere->m_Color);

		deviceCommandContext->DrawIndexed(0, m->sphereIndexes.size(), 0);

		g_Renderer.GetStats().m_DrawCalls++;
		g_Renderer.GetStats().m_OverlayTris = m->sphereIndexes.size()/3;
	}

	deviceCommandContext->EndPass();
}
