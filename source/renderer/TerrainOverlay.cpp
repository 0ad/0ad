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

#include "TerrainOverlay.h"

#include "graphics/Color.h"
#include "graphics/ShaderManager.h"
#include "graphics/ShaderProgram.h"
#include "graphics/Terrain.h"
#include "lib/bits.h"
#include "lib/ogl.h"
#include "maths/MathUtil.h"
#include "ps/CStrInternStatic.h"
#include "ps/Game.h"
#include "ps/Profile.h"
#include "ps/World.h"
#include "renderer/backend/gl/Device.h"
#include "renderer/Renderer.h"
#include "renderer/SceneRenderer.h"
#include "renderer/TerrainRenderer.h"
#include "simulation2/system/SimContext.h"

#include <algorithm>

// Global overlay list management:

static std::vector<std::pair<ITerrainOverlay*, int> > g_TerrainOverlayList;

ITerrainOverlay::ITerrainOverlay(int priority)
{
	// Add to global list of overlays
	g_TerrainOverlayList.emplace_back(this, priority);
	// Sort by overlays by priority. Do stable sort so that adding/removing
	// overlays doesn't randomly disturb all the existing ones (which would
	// be noticeable if they have the same priority and overlap).
	std::stable_sort(g_TerrainOverlayList.begin(), g_TerrainOverlayList.end(),
		[](const std::pair<ITerrainOverlay*, int>& a, const std::pair<ITerrainOverlay*, int>& b) {
			return a.second < b.second;
		});
}

ITerrainOverlay::~ITerrainOverlay()
{
	std::vector<std::pair<ITerrainOverlay*, int> >::iterator newEnd =
		std::remove_if(g_TerrainOverlayList.begin(), g_TerrainOverlayList.end(),
			[this](const std::pair<ITerrainOverlay*, int>& a) { return a.first == this; });
	g_TerrainOverlayList.erase(newEnd, g_TerrainOverlayList.end());
}


void ITerrainOverlay::RenderOverlaysBeforeWater(
	Renderer::Backend::GL::CDeviceCommandContext* deviceCommandContext)
{
	if (g_TerrainOverlayList.empty())
		return;

	PROFILE3_GPU("terrain overlays (before)");
	GPU_SCOPED_LABEL(deviceCommandContext, "Render terrain overlays before water");

	for (size_t i = 0; i < g_TerrainOverlayList.size(); ++i)
		g_TerrainOverlayList[i].first->RenderBeforeWater(deviceCommandContext);
}

void ITerrainOverlay::RenderOverlaysAfterWater(
	Renderer::Backend::GL::CDeviceCommandContext* deviceCommandContext, int cullGroup)
{
	if (g_TerrainOverlayList.empty())
		return;

	PROFILE3_GPU("terrain overlays (after)");
	GPU_SCOPED_LABEL(deviceCommandContext, "Render terrain overlays after water");

	for (size_t i = 0; i < g_TerrainOverlayList.size(); ++i)
		g_TerrainOverlayList[i].first->RenderAfterWater(deviceCommandContext, cullGroup);
}

//////////////////////////////////////////////////////////////////////////

TerrainOverlay::TerrainOverlay(const CSimContext& simContext, int priority /* = 100 */)
	: ITerrainOverlay(priority), m_Terrain(&simContext.GetTerrain())
{
}

void TerrainOverlay::StartRender()
{
}

void TerrainOverlay::EndRender()
{
}

void TerrainOverlay::GetTileExtents(
	ssize_t& min_i_inclusive, ssize_t& min_j_inclusive,
	ssize_t& max_i_inclusive, ssize_t& max_j_inclusive)
{
	// Default to whole map
	min_i_inclusive = min_j_inclusive = 0;
	max_i_inclusive = max_j_inclusive = m_Terrain->GetTilesPerSide()-1;
}

void TerrainOverlay::RenderBeforeWater(
	Renderer::Backend::GL::CDeviceCommandContext* deviceCommandContext)
{
	if (!m_Terrain)
		return; // should never happen, but let's play it safe

#if CONFIG2_GLES
	UNUSED2(deviceCommandContext);
#warning TODO: implement TerrainOverlay::RenderOverlays for GLES
#else
	// To ensure that outlines are drawn on top of the terrain correctly (and
	// don't Z-fight and flicker nastily), draw them as QUADS with the LINE
	// PolygonMode, and use PolygonOffset to pull them towards the camera.
	// (See e.g. http://www.opengl.org/resources/faq/technical/polygonoffset.htm)
	glPolygonOffset(-1.f, -1.f);
	//glEnable(GL_POLYGON_OFFSET_LINE);
	glEnable(GL_POLYGON_OFFSET_FILL);

	StartRender();

	ssize_t min_i, min_j, max_i, max_j;
	GetTileExtents(min_i, min_j, max_i, max_j);
	// Clamp the min to 0, but the max to -1 - so tile -1 can never be rendered,
	// but if unclamped_max<0 then no tiles at all will be rendered. And the same
	// for the upper limit.
	min_i = Clamp<ssize_t>(min_i, 0, m_Terrain->GetTilesPerSide());
	min_j = Clamp<ssize_t>(min_j, 0, m_Terrain->GetTilesPerSide());
	max_i = Clamp<ssize_t>(max_i, -1, m_Terrain->GetTilesPerSide()-1);
	max_j = Clamp<ssize_t>(max_j, -1, m_Terrain->GetTilesPerSide()-1);

	for (m_j = min_j; m_j <= max_j; ++m_j)
		for (m_i = min_i; m_i <= max_i; ++m_i)
			ProcessTile(deviceCommandContext, m_i, m_j);

	EndRender();

	//glDisable(GL_POLYGON_OFFSET_LINE);
	glDisable(GL_POLYGON_OFFSET_FILL);
#endif
}

void TerrainOverlay::RenderTile(
	Renderer::Backend::GL::CDeviceCommandContext* deviceCommandContext,
	const CColor& color, bool drawHidden)
{
	RenderTile(deviceCommandContext, color, drawHidden, m_i, m_j);
}

void TerrainOverlay::RenderTile(
	Renderer::Backend::GL::CDeviceCommandContext* deviceCommandContext,
	const CColor& color, bool drawHidden, ssize_t i, ssize_t j)
{
	// TODO: unnecessary computation calls has been removed but we should use
	// a vertex buffer or a vertex shader with a texture.
	// Not sure if it's possible on old OpenGL.

#if CONFIG2_GLES
	UNUSED2(deviceCommandContext);
	UNUSED2(color);
	UNUSED2(drawHidden);
	UNUSED2(i);
	UNUSED2(j);
	#warning TODO: implement TerrainOverlay::RenderTile for GLES
#else

	CVector3D pos[2][2];
	for (int di = 0; di < 2; ++di)
		for (int dj = 0; dj < 2; ++dj)
			m_Terrain->CalcPosition(i + di, j + dj, pos[di][dj]);

	std::vector<float> vertices;
#define ADD(position) \
	vertices.emplace_back((position).X); \
	vertices.emplace_back((position).Y); \
	vertices.emplace_back((position).Z);

	if (m_Terrain->GetTriangulationDir(i, j))
	{
		ADD(pos[0][0]);
		ADD(pos[1][0]);
		ADD(pos[0][1]);

		ADD(pos[1][0]);
		ADD(pos[1][1]);
		ADD(pos[0][1]);
	}
	else
	{
		ADD(pos[0][0]);
		ADD(pos[1][0]);
		ADD(pos[1][1]);

		ADD(pos[1][1]);
		ADD(pos[0][1]);
		ADD(pos[0][0]);
	}
#undef ADD

	CShaderTechniquePtr overlayTech =
		g_Renderer.GetShaderManager().LoadEffect(str_debug_line);
	Renderer::Backend::GraphicsPipelineStateDesc pipelineStateDesc =
		overlayTech->GetGraphicsPipelineStateDesc();
	pipelineStateDesc.depthStencilState.depthTestEnabled = !drawHidden;
	pipelineStateDesc.blendState.enabled = true;
	pipelineStateDesc.blendState.srcColorBlendFactor = pipelineStateDesc.blendState.srcAlphaBlendFactor =
		Renderer::Backend::BlendFactor::SRC_ALPHA;
	pipelineStateDesc.blendState.dstColorBlendFactor = pipelineStateDesc.blendState.dstAlphaBlendFactor =
		Renderer::Backend::BlendFactor::ONE_MINUS_SRC_ALPHA;
	pipelineStateDesc.blendState.colorBlendOp = pipelineStateDesc.blendState.alphaBlendOp =
		Renderer::Backend::BlendOp::ADD;
	pipelineStateDesc.rasterizationState.cullMode =
		drawHidden ? Renderer::Backend::CullMode::NONE : Renderer::Backend::CullMode::BACK;
	overlayTech->BeginPass();
	deviceCommandContext->SetGraphicsPipelineState(pipelineStateDesc);

	CShaderProgramPtr overlayShader = overlayTech->GetShader();

	overlayShader->Uniform(str_transform, g_Renderer.GetSceneRenderer().GetViewCamera().GetViewProjection());
	overlayShader->Uniform(str_color, color);

	overlayShader->VertexPointer(3, GL_FLOAT, 0, vertices.data());
	overlayShader->AssertPointersBound();

	glDrawArrays(GL_TRIANGLES, 0, vertices.size() / 3);

	overlayTech->EndPass();
#endif
}

void TerrainOverlay::RenderTileOutline(
	Renderer::Backend::GL::CDeviceCommandContext* deviceCommandContext,
	const CColor& color, bool drawHidden)
{
	RenderTileOutline(deviceCommandContext, color, drawHidden, m_i, m_j);
}

void TerrainOverlay::RenderTileOutline(
	Renderer::Backend::GL::CDeviceCommandContext* deviceCommandContext,
	const CColor& color, bool drawHidden, ssize_t i, ssize_t j)
{
#if CONFIG2_GLES
	UNUSED2(deviceCommandContext);
	UNUSED2(color);
	UNUSED2(lineWidth);
	UNUSED2(drawHidden);
	UNUSED2(i);
	UNUSED2(j);
	#warning TODO: implement TerrainOverlay::RenderTileOutline for GLES
#else

	std::vector<float> vertices;
#define ADD(i, j) \
	m_Terrain->CalcPosition(i, j, position); \
	vertices.emplace_back(position.X); \
	vertices.emplace_back(position.Y); \
	vertices.emplace_back(position.Z);

	CVector3D position;
	ADD(i, j);
	ADD(i + 1, j);
	ADD(i + 1, j + 1);
	ADD(i, j);
	ADD(i + 1, j + 1);
	ADD(i, j + 1);
#undef ADD

	CShaderTechniquePtr overlayTech =
		g_Renderer.GetShaderManager().LoadEffect(str_debug_line);
	Renderer::Backend::GraphicsPipelineStateDesc pipelineStateDesc =
		overlayTech->GetGraphicsPipelineStateDesc();
	pipelineStateDesc.depthStencilState.depthTestEnabled = !drawHidden;
	pipelineStateDesc.blendState.enabled = true;
	pipelineStateDesc.blendState.srcColorBlendFactor = pipelineStateDesc.blendState.srcAlphaBlendFactor =
		Renderer::Backend::BlendFactor::SRC_ALPHA;
	pipelineStateDesc.blendState.dstColorBlendFactor = pipelineStateDesc.blendState.dstAlphaBlendFactor =
		Renderer::Backend::BlendFactor::ONE_MINUS_SRC_ALPHA;
	pipelineStateDesc.blendState.colorBlendOp = pipelineStateDesc.blendState.alphaBlendOp =
		Renderer::Backend::BlendOp::ADD;
	pipelineStateDesc.rasterizationState.cullMode =
		drawHidden ? Renderer::Backend::CullMode::NONE : Renderer::Backend::CullMode::BACK;
	pipelineStateDesc.rasterizationState.polygonMode = Renderer::Backend::PolygonMode::LINE;
	overlayTech->BeginPass();
	deviceCommandContext->SetGraphicsPipelineState(pipelineStateDesc);

	const CShaderProgramPtr& overlayShader = overlayTech->GetShader();

	overlayShader->Uniform(str_transform, g_Renderer.GetSceneRenderer().GetViewCamera().GetViewProjection());
	overlayShader->Uniform(str_color, color);

	overlayShader->VertexPointer(3, GL_FLOAT, 0, vertices.data());
	overlayShader->AssertPointersBound();

	glDrawArrays(GL_TRIANGLES, 0, vertices.size() / 3);

	overlayTech->EndPass();
#endif
}

//////////////////////////////////////////////////////////////////////////

TerrainTextureOverlay::TerrainTextureOverlay(float texelsPerTile, int priority) :
	ITerrainOverlay(priority), m_TexelsPerTile(texelsPerTile)
{
}

TerrainTextureOverlay::~TerrainTextureOverlay() = default;

void TerrainTextureOverlay::RenderAfterWater(
	Renderer::Backend::GL::CDeviceCommandContext* deviceCommandContext, int cullGroup)
{
	CTerrain* terrain = g_Game->GetWorld()->GetTerrain();

	ssize_t w = (ssize_t)(terrain->GetTilesPerSide() * m_TexelsPerTile);
	ssize_t h = (ssize_t)(terrain->GetTilesPerSide() * m_TexelsPerTile);

	const uint32_t requiredWidth = round_up_to_pow2(w);
	const uint32_t requiredHeight = round_up_to_pow2(h);

	// Recreate the texture with new size if necessary
	if (!m_Texture || m_Texture->GetWidth() != requiredWidth || m_Texture->GetHeight() != requiredHeight)
	{
		m_Texture = deviceCommandContext->GetDevice()->CreateTexture2D("TerrainOverlayTexture",
			Renderer::Backend::Format::R8G8B8A8, requiredWidth, requiredHeight,
			Renderer::Backend::Sampler::MakeDefaultSampler(
				Renderer::Backend::Sampler::Filter::NEAREST,
				Renderer::Backend::Sampler::AddressMode::CLAMP_TO_EDGE));
	}

	u8* data = (u8*)calloc(w * h, 4);
	BuildTextureRGBA(data, w, h);

	deviceCommandContext->UploadTextureRegion(
		m_Texture.get(), Renderer::Backend::Format::R8G8B8A8, data, w * h * 4, 0, 0, w, h);

	free(data);

	CMatrix3D matrix;
	matrix.SetZero();
	matrix._11 = m_TexelsPerTile / (m_Texture->GetWidth() * TERRAIN_TILE_SIZE);
	matrix._23 = m_TexelsPerTile / (m_Texture->GetHeight() * TERRAIN_TILE_SIZE);
	matrix._44 = 1;

	g_Renderer.GetSceneRenderer().GetTerrainRenderer().RenderTerrainOverlayTexture(
		deviceCommandContext, cullGroup, matrix, m_Texture.get());
}

SColor4ub TerrainTextureOverlay::GetColor(size_t idx, u8 alpha) const
{
	static u8 colors[][3] =
	{
		{ 255, 0, 0 },
		{ 0, 255, 0 },
		{ 0, 0, 255 },
		{ 255, 255, 0 },
		{ 255, 0, 255 },
		{ 0, 255, 255 },
		{ 255, 255, 255 },

		{ 127, 0, 0 },
		{ 0, 127, 0 },
		{ 0, 0, 127 },
		{ 127, 127, 0 },
		{ 127, 0, 127 },
		{ 0, 127, 127 },
		{ 127, 127, 127},

		{ 255, 127, 0 },
		{ 127, 255, 0 },
		{ 255, 0, 127 },
		{ 127, 0, 255},
		{ 0, 255, 127 },
		{ 0, 127, 255},
		{ 255, 127, 127},
		{ 127, 255, 127},
		{ 127, 127, 255},

		{ 127, 255, 255 },
		{ 255, 127, 255 },
		{ 255, 255, 127 },
	};

	size_t c = idx % ARRAY_SIZE(colors);
	return SColor4ub(colors[c][0], colors[c][1], colors[c][2], alpha);
}
