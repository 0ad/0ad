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

#include "MiniMapTexture.h"

#include "graphics/GameView.h"
#include "graphics/LOSTexture.h"
#include "graphics/MiniPatch.h"
#include "graphics/ShaderManager.h"
#include "graphics/ShaderProgramPtr.h"
#include "graphics/Terrain.h"
#include "graphics/TerrainTextureEntry.h"
#include "graphics/TerrainTextureManager.h"
#include "graphics/TerritoryTexture.h"
#include "graphics/TextureManager.h"
#include "lib/bits.h"
#include "lib/timer.h"
#include "ps/ConfigDB.h"
#include "ps/CStrInternStatic.h"
#include "ps/Filesystem.h"
#include "ps/Game.h"
#include "ps/World.h"
#include "ps/XML/Xeromyces.h"
#include "renderer/backend/gl/Device.h"
#include "renderer/Renderer.h"
#include "renderer/RenderingOptions.h"
#include "renderer/SceneRenderer.h"
#include "renderer/WaterManager.h"
#include "scriptinterface/Object.h"
#include "simulation2/Simulation2.h"
#include "simulation2/components/ICmpMinimap.h"
#include "simulation2/components/ICmpRangeManager.h"
#include "simulation2/system/ParamNode.h"

namespace
{

// Set max drawn entities to UINT16_MAX for now, which is more than enough
// TODO: we should be cleverer about drawing them to reduce clutter
const u16 MAX_ENTITIES_DRAWN = 65535;

const size_t FINAL_TEXTURE_SIZE = 512;

unsigned int ScaleColor(unsigned int color, float x)
{
	unsigned int r = unsigned(float(color & 0xff) * x);
	unsigned int g = unsigned(float((color >> 8) & 0xff) * x);
	unsigned int b = unsigned(float((color >> 16) & 0xff) * x);
	return (0xff000000 | b | g << 8 | r << 16);
}

void DrawTexture(CShaderProgramPtr shader)
{
	const float quadUVs[] =
	{
		0.0f, 0.0f,
		1.0f, 0.0f,
		1.0f, 1.0f,

		1.0f, 1.0f,
		0.0f, 1.0f,
		0.0f, 0.0f
	};
	const float quadVertices[] =
	{
		-1.0f, -1.0f, 0.0f,
		1.0f, -1.0f, 0.0f,
		1.0f, 1.0f, 0.0f,

		1.0f, 1.0f, 0.0f,
		-1.0f, 1.0f, 0.0f,
		-1.0f, -1.0f, 0.0f
	};

	shader->TexCoordPointer(GL_TEXTURE0, 2, GL_FLOAT, 0, quadUVs);
	shader->VertexPointer(3, GL_FLOAT, 0, quadVertices);
	shader->AssertPointersBound();

	glDrawArrays(GL_TRIANGLES, 0, 6);
}

struct MinimapUnitVertex
{
	// This struct is copyable for convenience and because to move is to copy for primitives.
	u8 r, g, b, a;
	float x, y;
};

// Adds a vertex to the passed VertexArray
static void inline addVertex(const MinimapUnitVertex& v,
	VertexArrayIterator<u8[4]>& attrColor,
	VertexArrayIterator<float[2]>& attrPos)
{
	(*attrColor)[0] = v.r;
	(*attrColor)[1] = v.g;
	(*attrColor)[2] = v.b;
	(*attrColor)[3] = v.a;
	++attrColor;

	(*attrPos)[0] = v.x;
	(*attrPos)[1] = v.y;

	++attrPos;
}

} // anonymous namespace

CMiniMapTexture::CMiniMapTexture(CSimulation2& simulation)
	: m_Simulation(simulation), m_IndexArray(false),
	m_VertexArray(Renderer::Backend::GL::CBuffer::Type::VERTEX, true)
{
	// Register Relax NG validator.
	CXeromyces::AddValidator(g_VFS, "pathfinder", "simulation/data/pathfinder.rng");

	m_ShallowPassageHeight = GetShallowPassageHeight();

	double blinkDuration = 1.0;
	// Tests won't have config initialised
	if (CConfigDB::IsInitialised())
	{
		CFG_GET_VAL("gui.session.minimap.blinkduration", blinkDuration);
		CFG_GET_VAL("gui.session.minimap.pingduration", m_PingDuration);
	}
	m_HalfBlinkDuration = blinkDuration / 2.0;

	m_AttributePos.type = GL_FLOAT;
	m_AttributePos.elems = 2;
	m_VertexArray.AddAttribute(&m_AttributePos);

	m_AttributeColor.type = GL_UNSIGNED_BYTE;
	m_AttributeColor.elems = 4;
	m_VertexArray.AddAttribute(&m_AttributeColor);

	m_VertexArray.SetNumberOfVertices(MAX_ENTITIES_DRAWN);
	m_VertexArray.Layout();

	m_IndexArray.SetNumberOfVertices(MAX_ENTITIES_DRAWN);
	m_IndexArray.Layout();
	VertexArrayIterator<u16> index = m_IndexArray.GetIterator();
	for (u16 i = 0; i < MAX_ENTITIES_DRAWN; ++i)
		*index++ = i;
	m_IndexArray.Upload();
	m_IndexArray.FreeBackingStore();

	VertexArrayIterator<float[2]> attrPos = m_AttributePos.GetIterator<float[2]>();
	VertexArrayIterator<u8[4]> attrColor = m_AttributeColor.GetIterator<u8[4]>();
	for (u16 i = 0; i < MAX_ENTITIES_DRAWN; ++i)
	{
		(*attrColor)[0] = 0;
		(*attrColor)[1] = 0;
		(*attrColor)[2] = 0;
		(*attrColor)[3] = 0;
		++attrColor;

		(*attrPos)[0] = -10000.0f;
		(*attrPos)[1] = -10000.0f;

		++attrPos;

	}
	m_VertexArray.Upload();
}

CMiniMapTexture::~CMiniMapTexture()
{
	DestroyTextures();
}

void CMiniMapTexture::Update(const float UNUSED(deltaRealTime))
{
	if (m_WaterHeight != g_Renderer.GetSceneRenderer().GetWaterManager().m_WaterHeight)
	{
		m_TerrainTextureDirty = true;
		m_FinalTextureDirty = true;
	}
}

void CMiniMapTexture::Render(Renderer::Backend::GL::CDeviceCommandContext* deviceCommandContext)
{
	const CTerrain* terrain = g_Game->GetWorld()->GetTerrain();
	if (!terrain)
		return;

	if (!m_TerrainTexture)
		CreateTextures(deviceCommandContext, terrain);

	if (m_TerrainTextureDirty)
		RebuildTerrainTexture(deviceCommandContext, terrain);

	RenderFinalTexture(deviceCommandContext);
}

void CMiniMapTexture::CreateTextures(
	Renderer::Backend::GL::CDeviceCommandContext* deviceCommandContext, const CTerrain* terrain)
{
	DestroyTextures();

	m_MapSize = terrain->GetVerticesPerSide();
	const size_t textureSize = round_up_to_pow2(static_cast<size_t>(m_MapSize));

	const Renderer::Backend::Sampler::Desc defaultSamplerDesc =
		Renderer::Backend::Sampler::MakeDefaultSampler(
			Renderer::Backend::Sampler::Filter::LINEAR,
			Renderer::Backend::Sampler::AddressMode::CLAMP_TO_EDGE);

	// Create terrain texture
	m_TerrainTexture = deviceCommandContext->GetDevice()->CreateTexture2D("MiniMapTerrainTexture",
		Renderer::Backend::Format::R8G8B8A8, textureSize, textureSize, defaultSamplerDesc);

	// Initialise texture with solid black, for the areas we don't
	// overwrite with uploading later.
	std::unique_ptr<u32[]> texData = std::make_unique<u32[]>(textureSize * textureSize);
	for (size_t i = 0; i < textureSize * textureSize; ++i)
		texData[i] = 0xFF000000;
	deviceCommandContext->UploadTexture(
		m_TerrainTexture.get(), Renderer::Backend::Format::R8G8B8A8,
		texData.get(), textureSize * textureSize * 4);
	texData.reset();

	m_TerrainData = std::make_unique<u32[]>((m_MapSize - 1) * (m_MapSize - 1));

	m_FinalTexture = deviceCommandContext->GetDevice()->CreateTexture2D("MiniMapFinalTexture",
		Renderer::Backend::Format::R8G8B8A8, FINAL_TEXTURE_SIZE, FINAL_TEXTURE_SIZE, defaultSamplerDesc);

	m_FinalTextureFramebuffer = Renderer::Backend::GL::CFramebuffer::Create(
		m_FinalTexture.get(), nullptr);
	ENSURE(m_FinalTextureFramebuffer);
}

void CMiniMapTexture::DestroyTextures()
{
	m_TerrainTexture.reset();
	m_FinalTexture.reset();
	m_TerrainData.reset();
}

void CMiniMapTexture::RebuildTerrainTexture(
	Renderer::Backend::GL::CDeviceCommandContext* deviceCommandContext,
	const CTerrain* terrain)
{
	const u32 x = 0;
	const u32 y = 0;
	const u32 width = m_MapSize - 1;
	const u32 height = m_MapSize - 1;

	m_WaterHeight = g_Renderer.GetSceneRenderer().GetWaterManager().m_WaterHeight;
	m_TerrainTextureDirty = false;

	for (u32 j = 0; j < height; ++j)
	{
		u32* dataPtr = m_TerrainData.get() + ((y + j) * width) + x;
		for (u32 i = 0; i < width; ++i)
		{
			const float avgHeight = ( terrain->GetVertexGroundLevel((int)i, (int)j)
					+ terrain->GetVertexGroundLevel((int)i+1, (int)j)
					+ terrain->GetVertexGroundLevel((int)i, (int)j+1)
					+ terrain->GetVertexGroundLevel((int)i+1, (int)j+1)
				) / 4.0f;

			if (avgHeight < m_WaterHeight && avgHeight > m_WaterHeight - m_ShallowPassageHeight)
			{
				// shallow water
				*dataPtr++ = 0xffc09870;
			}
			else if (avgHeight < m_WaterHeight)
			{
				// Set water as constant color for consistency on different maps
				*dataPtr++ = 0xffa07850;
			}
			else
			{
				int hmap = ((int)terrain->GetHeightMap()[(y + j) * m_MapSize + x + i]) >> 8;
				int val = (hmap / 3) + 170;

				u32 color = 0xFFFFFFFF;

				CMiniPatch* mp = terrain->GetTile(x + i, y + j);
				if (mp)
				{
					CTerrainTextureEntry* tex = mp->GetTextureEntry();
					if (tex)
					{
						// If the texture can't be loaded yet, set the dirty flags
						// so we'll try regenerating the terrain texture again soon
						if(!tex->GetTexture()->TryLoad())
							m_TerrainTextureDirty = true;

						color = tex->GetBaseColor();
					}
				}

				*dataPtr++ = ScaleColor(color, float(val) / 255.0f);
			}
		}
	}

	// Upload the texture
	deviceCommandContext->UploadTextureRegion(
		m_TerrainTexture.get(), Renderer::Backend::Format::R8G8B8A8,
		m_TerrainData.get(), width * height * 4, 0, 0, width, height);
}

void CMiniMapTexture::RenderFinalTexture(
	Renderer::Backend::GL::CDeviceCommandContext* deviceCommandContext)
{
	// only update 2x / second
	// (note: since units only move a few pixels per second on the minimap,
	// we can get away with infrequent updates; this is slow)
	// TODO: Update all but camera at same speed as simulation
	const double currentTime = timer_Time();
	const bool doUpdate = (currentTime - m_LastFinalTextureUpdate > 0.5) || m_FinalTextureDirty;
	if (doUpdate)
		m_LastFinalTextureUpdate = currentTime;
	else
		return;
	m_FinalTextureDirty = false;

	GPU_SCOPED_LABEL(deviceCommandContext, "Render minimap texture");
	deviceCommandContext->SetFramebuffer(m_FinalTextureFramebuffer.get());

	const SViewPort oldViewPort = g_Renderer.GetViewport();
	const SViewPort viewPort = { 0, 0, FINAL_TEXTURE_SIZE, FINAL_TEXTURE_SIZE };
	g_Renderer.SetViewport(viewPort);

	CmpPtr<ICmpRangeManager> cmpRangeManager(m_Simulation, SYSTEM_ENTITY);
	ENSURE(cmpRangeManager);
	CLOSTexture& losTexture = g_Game->GetView()->GetLOSTexture();

	const float invTileMapSize = 1.0f / static_cast<float>(TERRAIN_TILE_SIZE * m_MapSize);
	const float texCoordMax = m_TerrainTexture ? static_cast<float>(m_MapSize - 1) / m_TerrainTexture->GetWidth() : 1.0f;

	CShaderProgramPtr shader;
	CShaderTechniquePtr tech;

	CShaderDefines baseDefines;
	baseDefines.Add(str_MINIMAP_BASE, str_1);

	tech = g_Renderer.GetShaderManager().LoadEffect(str_minimap, baseDefines);
	Renderer::Backend::GraphicsPipelineStateDesc pipelineStateDesc =
		tech->GetGraphicsPipelineStateDesc();
	tech->BeginPass();
	deviceCommandContext->SetGraphicsPipelineState(pipelineStateDesc);
	shader = tech->GetShader();

	if (m_TerrainTexture)
		shader->BindTexture(str_baseTex, m_TerrainTexture.get());

	CMatrix3D baseTransform;
	baseTransform.SetIdentity();
	CMatrix3D baseTextureTransform;
	baseTextureTransform.SetIdentity();

	CMatrix3D terrainTransform;
	terrainTransform.SetIdentity();
	terrainTransform.Scale(texCoordMax, texCoordMax, 1.0f);
	shader->Uniform(str_transform, baseTransform);
	shader->Uniform(str_textureTransform, terrainTransform);

	if (m_TerrainTexture)
		DrawTexture(shader);

	pipelineStateDesc.blendState.enabled = true;
	pipelineStateDesc.blendState.srcColorBlendFactor = pipelineStateDesc.blendState.srcAlphaBlendFactor =
		Renderer::Backend::BlendFactor::SRC_ALPHA;
	pipelineStateDesc.blendState.dstColorBlendFactor = pipelineStateDesc.blendState.dstAlphaBlendFactor =
		Renderer::Backend::BlendFactor::ONE_MINUS_SRC_ALPHA;
	pipelineStateDesc.blendState.colorBlendOp = pipelineStateDesc.blendState.alphaBlendOp =
		Renderer::Backend::BlendOp::ADD;
	pipelineStateDesc.blendState.colorWriteMask =
		Renderer::Backend::ColorWriteMask::RED |
		Renderer::Backend::ColorWriteMask::GREEN |
		Renderer::Backend::ColorWriteMask::BLUE;
	deviceCommandContext->SetGraphicsPipelineState(pipelineStateDesc);

	// Draw territory boundaries
	CTerritoryTexture& territoryTexture = g_Game->GetView()->GetTerritoryTexture();

	shader->BindTexture(str_baseTex, territoryTexture.GetTexture());
	shader->Uniform(str_transform, baseTransform);
	shader->Uniform(str_textureTransform, territoryTexture.GetMinimapTextureMatrix());

	DrawTexture(shader);

	pipelineStateDesc.blendState.enabled = false;
	pipelineStateDesc.blendState.colorWriteMask =
		Renderer::Backend::ColorWriteMask::ALPHA;
	deviceCommandContext->SetGraphicsPipelineState(pipelineStateDesc);

	shader->BindTexture(str_baseTex, losTexture.GetTexture());
	shader->Uniform(str_transform, baseTransform);
	shader->Uniform(str_textureTransform, losTexture.GetMinimapTextureMatrix());

	DrawTexture(shader);

	tech->EndPass();

	CShaderDefines pointDefines;
	pointDefines.Add(str_MINIMAP_POINT, str_1);
	tech = g_Renderer.GetShaderManager().LoadEffect(str_minimap, pointDefines);
	tech->BeginPass();
	deviceCommandContext->SetGraphicsPipelineState(
		tech->GetGraphicsPipelineStateDesc());
	shader = tech->GetShader();
	shader->Uniform(str_transform, baseTransform);
	shader->Uniform(str_pointSize, 9.0f);

	CMatrix3D unitMatrix;
	unitMatrix.SetIdentity();
	// Convert world space coordinates into [0, 2].
	const float unitScale = invTileMapSize;
	unitMatrix.Scale(unitScale * 2.0f, unitScale * 2.0f, 1.0f);
	// Offset the coordinates to [-1, 1].
	unitMatrix.Translate(CVector3D(-1.0f, -1.0f, 0.0f));
	shader->Uniform(str_transform, unitMatrix);

	CSimulation2::InterfaceList ents = m_Simulation.GetEntitiesWithInterface(IID_Minimap);

	if (doUpdate)
	{
		VertexArrayIterator<float[2]> attrPos = m_AttributePos.GetIterator<float[2]>();
		VertexArrayIterator<u8[4]> attrColor = m_AttributeColor.GetIterator<u8[4]>();

		m_EntitiesDrawn = 0;
		MinimapUnitVertex v;
		std::vector<MinimapUnitVertex> pingingVertices;
		pingingVertices.reserve(MAX_ENTITIES_DRAWN / 2);

		if (currentTime > m_NextBlinkTime)
		{
			m_BlinkState = !m_BlinkState;
			m_NextBlinkTime = currentTime + m_HalfBlinkDuration;
		}

		entity_pos_t posX, posZ;
		for (CSimulation2::InterfaceList::const_iterator it = ents.begin(); it != ents.end(); ++it)
		{
			ICmpMinimap* cmpMinimap = static_cast<ICmpMinimap*>(it->second);
			if (cmpMinimap->GetRenderData(v.r, v.g, v.b, posX, posZ))
			{
				LosVisibility vis = cmpRangeManager->GetLosVisibility(it->first, m_Simulation.GetSimContext().GetCurrentDisplayedPlayer());
				if (vis != LosVisibility::HIDDEN)
				{
					v.a = 255;
					v.x = posX.ToFloat();
					v.y = posZ.ToFloat();

					// Check minimap pinging to indicate something
					if (m_BlinkState && cmpMinimap->CheckPing(currentTime, m_PingDuration))
					{
						v.r = 255; // ping color is white
						v.g = 255;
						v.b = 255;
						pingingVertices.push_back(v);
					}
					else
					{
						addVertex(v, attrColor, attrPos);
						++m_EntitiesDrawn;
					}
				}
			}
		}

		// Add the pinged vertices at the end, so they are drawn on top
		for (const MinimapUnitVertex& vertex : pingingVertices)
		{
			addVertex(vertex, attrColor, attrPos);
			++m_EntitiesDrawn;
		}

		ENSURE(m_EntitiesDrawn < MAX_ENTITIES_DRAWN);
		m_VertexArray.Upload();
	}

	m_VertexArray.PrepareForRendering();

	if (m_EntitiesDrawn > 0)
	{
		Renderer::Backend::GL::CDeviceCommandContext::ScissorRect scissorRect;
		scissorRect.x = scissorRect.y = 1;
		scissorRect.width = scissorRect.height = FINAL_TEXTURE_SIZE - 2;
		deviceCommandContext->SetScissors(1, &scissorRect);
#if !CONFIG2_GLES
		glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);
#endif

		u8* indexBase = m_IndexArray.Bind(deviceCommandContext);
		u8* base = m_VertexArray.Bind(deviceCommandContext);
		const GLsizei stride = (GLsizei)m_VertexArray.GetStride();

		shader->VertexPointer(2, GL_FLOAT, stride, base + m_AttributePos.offset);
		shader->ColorPointer(4, GL_UNSIGNED_BYTE, stride, base + m_AttributeColor.offset);
		shader->AssertPointersBound();

		glDrawElements(GL_POINTS, (GLsizei)(m_EntitiesDrawn), GL_UNSIGNED_SHORT, indexBase);

		g_Renderer.GetStats().m_DrawCalls++;
		CVertexBuffer::Unbind(deviceCommandContext);

#if !CONFIG2_GLES
		glDisable(GL_VERTEX_PROGRAM_POINT_SIZE);
#endif
		deviceCommandContext->SetScissors(0, nullptr);
	}

	tech->EndPass();
	deviceCommandContext->SetFramebuffer(
		deviceCommandContext->GetDevice()->GetCurrentBackbuffer());
	g_Renderer.SetViewport(oldViewPort);
}

// static
float CMiniMapTexture::GetShallowPassageHeight()
{
	float shallowPassageHeight = 0.0f;
	CParamNode externalParamNode;
	CParamNode::LoadXML(externalParamNode, L"simulation/data/pathfinder.xml", "pathfinder");
	const CParamNode pathingSettings = externalParamNode.GetChild("Pathfinder").GetChild("PassabilityClasses");
	if (pathingSettings.GetChild("default").IsOk() && pathingSettings.GetChild("default").GetChild("MaxWaterDepth").IsOk())
		shallowPassageHeight = pathingSettings.GetChild("default").GetChild("MaxWaterDepth").ToFloat();
	return shallowPassageHeight;
}
