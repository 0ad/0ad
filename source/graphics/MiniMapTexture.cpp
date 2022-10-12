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
#include "lib/hash.h"
#include "lib/timer.h"
#include "maths/MathUtil.h"
#include "maths/Vector2D.h"
#include "ps/ConfigDB.h"
#include "ps/CStrInternStatic.h"
#include "ps/Filesystem.h"
#include "ps/Game.h"
#include "ps/Profile.h"
#include "ps/VideoMode.h"
#include "ps/World.h"
#include "ps/XML/Xeromyces.h"
#include "renderer/backend/IDevice.h"
#include "renderer/Renderer.h"
#include "renderer/RenderingOptions.h"
#include "renderer/SceneRenderer.h"
#include "renderer/WaterManager.h"
#include "scriptinterface/Object.h"
#include "simulation2/Simulation2.h"
#include "simulation2/components/ICmpMinimap.h"
#include "simulation2/components/ICmpRangeManager.h"
#include "simulation2/system/ParamNode.h"

#include <algorithm>
#include <array>
#include <cmath>

namespace
{

// Set max drawn entities to 64K / 4 for now, which is more than enough.
// 4 is the number of vertices per entity.
// TODO: we should be cleverer about drawing them to reduce clutter,
// f.e. use instancing.
constexpr size_t MAX_ENTITIES_DRAWN = 65536 / 4;

constexpr size_t MAX_ICON_COUNT = 256;
constexpr size_t MAX_UNIQUE_ICON_COUNT = 64;
constexpr size_t ICON_COMBINING_GRID_SIZE = 10;

constexpr size_t FINAL_TEXTURE_SIZE = 512;

unsigned int ScaleColor(unsigned int color, float x)
{
	unsigned int r = unsigned(float(color & 0xff) * x);
	unsigned int g = unsigned(float((color >> 8) & 0xff) * x);
	unsigned int b = unsigned(float((color >> 16) & 0xff) * x);
	return (0xff000000 | b | g << 8 | r << 16);
}

void DrawTexture(
	Renderer::Backend::IDeviceCommandContext* deviceCommandContext)
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
		-1.0f, -1.0f,
		1.0f, -1.0f,
		1.0f, 1.0f,

		1.0f, 1.0f,
		-1.0f, 1.0f,
		-1.0f, -1.0f,
	};

	deviceCommandContext->SetVertexAttributeFormat(
		Renderer::Backend::VertexAttributeStream::POSITION,
		Renderer::Backend::Format::R32G32_SFLOAT, 0, sizeof(float) * 2,
		Renderer::Backend::VertexAttributeRate::PER_VERTEX, 0);
	deviceCommandContext->SetVertexAttributeFormat(
		Renderer::Backend::VertexAttributeStream::UV0,
		Renderer::Backend::Format::R32G32_SFLOAT, 0, sizeof(float) * 2,
		Renderer::Backend::VertexAttributeRate::PER_VERTEX, 1);

	deviceCommandContext->SetVertexBufferData(
		0, quadVertices, std::size(quadVertices) * sizeof(quadVertices[0]));
	deviceCommandContext->SetVertexBufferData(
		1, quadUVs, std::size(quadUVs) * sizeof(quadUVs[0]));

	deviceCommandContext->Draw(0, 6);
}

struct MinimapUnitVertex
{
	// This struct is copyable for convenience and because to move is to copy for primitives.
	u8 r, g, b, a;
	CVector2D position;
};

// Adds a vertex to the passed VertexArray
inline void AddEntity(const MinimapUnitVertex& v,
	VertexArrayIterator<u8[4]>& attrColor,
	VertexArrayIterator<float[2]>& attrPos,
	const float entityRadius,
	const bool useInstancing)
{
	if (useInstancing)
	{
		(*attrColor)[0] = v.r;
		(*attrColor)[1] = v.g;
		(*attrColor)[2] = v.b;
		(*attrColor)[3] = v.a;
		++attrColor;

		(*attrPos)[0] = v.position.X;
		(*attrPos)[1] = v.position.Y;
		++attrPos;

		return;
	}

	const CVector2D offsets[4] =
	{
		{-entityRadius, 0.0f},
		{0.0f, -entityRadius},
		{entityRadius, 0.0f},
		{0.0f, entityRadius}
	};

	for (const CVector2D& offset : offsets)
	{
		(*attrColor)[0] = v.r;
		(*attrColor)[1] = v.g;
		(*attrColor)[2] = v.b;
		(*attrColor)[3] = v.a;
		++attrColor;

		(*attrPos)[0] = v.position.X + offset.X;
		(*attrPos)[1] = v.position.Y + offset.Y;
		++attrPos;
	}
}

} // anonymous namespace

size_t CMiniMapTexture::CellIconKeyHash::operator()(
	const CellIconKey& key) const
{
	size_t seed = 0;
	hash_combine(seed, key.path);
	hash_combine(seed, key.r);
	hash_combine(seed, key.g);
	hash_combine(seed, key.b);
	return seed;
}

bool CMiniMapTexture::CellIconKeyEqual::operator()(
	const CellIconKey& lhs, const CellIconKey& rhs) const
{
	return
		lhs.path == rhs.path &&
		lhs.r == rhs.r &&
		lhs.g == rhs.g &&
		lhs.b == rhs.b;
}

CMiniMapTexture::CMiniMapTexture(CSimulation2& simulation)
	: m_Simulation(simulation), m_IndexArray(false),
	m_VertexArray(Renderer::Backend::IBuffer::Type::VERTEX, true),
	m_InstanceVertexArray(Renderer::Backend::IBuffer::Type::VERTEX, false)
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

	m_AttributePos.format = Renderer::Backend::Format::R32G32_SFLOAT;
	m_VertexArray.AddAttribute(&m_AttributePos);

	m_AttributeColor.format = Renderer::Backend::Format::R8G8B8A8_UNORM;
	m_VertexArray.AddAttribute(&m_AttributeColor);

	m_VertexArray.SetNumberOfVertices(MAX_ENTITIES_DRAWN * 4);
	m_VertexArray.Layout();

	m_IndexArray.SetNumberOfVertices(MAX_ENTITIES_DRAWN * 6);
	m_IndexArray.Layout();
	VertexArrayIterator<u16> index = m_IndexArray.GetIterator();
	for (size_t i = 0; i < m_IndexArray.GetNumberOfVertices(); ++i)
		*index++ = 0;
	m_IndexArray.Upload();

	VertexArrayIterator<float[2]> attrPos = m_AttributePos.GetIterator<float[2]>();
	VertexArrayIterator<u8[4]> attrColor = m_AttributeColor.GetIterator<u8[4]>();
	for (size_t i = 0; i < m_VertexArray.GetNumberOfVertices(); ++i)
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

	if (g_VideoMode.GetBackendDevice()->GetCapabilities().instancing)
	{
		m_UseInstancing = true;

		const size_t numberOfCircleSegments = 8;

		m_InstanceAttributePosition.format = Renderer::Backend::Format::R32G32_SFLOAT;
		m_InstanceVertexArray.AddAttribute(&m_InstanceAttributePosition);

		m_InstanceVertexArray.SetNumberOfVertices(numberOfCircleSegments * 3);
		m_InstanceVertexArray.Layout();

		VertexArrayIterator<float[2]> attributePosition =
			m_InstanceAttributePosition.GetIterator<float[2]>();
		for (size_t segment = 0; segment < numberOfCircleSegments; ++segment)
		{
			const float currentAngle = static_cast<float>(segment) / numberOfCircleSegments * 2.0f * M_PI;
			const float nextAngle = static_cast<float>(segment + 1) / numberOfCircleSegments * 2.0f * M_PI;

			(*attributePosition)[0] = 0.0f;
			(*attributePosition)[1] = 0.0f;
			++attributePosition;

			(*attributePosition)[0] = std::cos(currentAngle);
			(*attributePosition)[1] = std::sin(currentAngle);
			++attributePosition;

			(*attributePosition)[0] = std::cos(nextAngle);
			(*attributePosition)[1] = std::sin(nextAngle);
			++attributePosition;
		}

		m_InstanceVertexArray.Upload();
		m_InstanceVertexArray.FreeBackingStore();
	}
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

void CMiniMapTexture::Render(
	Renderer::Backend::IDeviceCommandContext* deviceCommandContext,
	CLOSTexture& losTexture, CTerritoryTexture& territoryTexture)
{
	const CTerrain* terrain = g_Game->GetWorld()->GetTerrain();
	if (!terrain)
		return;

	if (!m_TerrainTexture)
		CreateTextures(deviceCommandContext, terrain);

	if (m_TerrainTextureDirty)
		RebuildTerrainTexture(deviceCommandContext, terrain);

	RenderFinalTexture(deviceCommandContext, losTexture, territoryTexture);
}

void CMiniMapTexture::CreateTextures(
	Renderer::Backend::IDeviceCommandContext* deviceCommandContext, const CTerrain* terrain)
{
	DestroyTextures();

	m_MapSize = terrain->GetVerticesPerSide();
	const size_t textureSize = round_up_to_pow2(static_cast<size_t>(m_MapSize));

	const Renderer::Backend::Sampler::Desc defaultSamplerDesc =
		Renderer::Backend::Sampler::MakeDefaultSampler(
			Renderer::Backend::Sampler::Filter::LINEAR,
			Renderer::Backend::Sampler::AddressMode::CLAMP_TO_EDGE);

	Renderer::Backend::IDevice* backendDevice = deviceCommandContext->GetDevice();

	// Create terrain texture
	m_TerrainTexture = backendDevice->CreateTexture2D("MiniMapTerrainTexture",
		Renderer::Backend::Format::R8G8B8A8_UNORM, textureSize, textureSize, defaultSamplerDesc);

	// Initialise texture with solid black, for the areas we don't
	// overwrite with uploading later.
	std::unique_ptr<u32[]> texData = std::make_unique<u32[]>(textureSize * textureSize);
	for (size_t i = 0; i < textureSize * textureSize; ++i)
		texData[i] = 0xFF000000;
	deviceCommandContext->UploadTexture(
		m_TerrainTexture.get(), Renderer::Backend::Format::R8G8B8A8_UNORM,
		texData.get(), textureSize * textureSize * 4);
	texData.reset();

	m_TerrainData = std::make_unique<u32[]>((m_MapSize - 1) * (m_MapSize - 1));

	m_FinalTexture = g_Renderer.GetTextureManager().WrapBackendTexture(
		backendDevice->CreateTexture2D("MiniMapFinalTexture",
			Renderer::Backend::Format::R8G8B8A8_UNORM,
			FINAL_TEXTURE_SIZE, FINAL_TEXTURE_SIZE, defaultSamplerDesc));

	m_FinalTextureFramebuffer = backendDevice->CreateFramebuffer("MiniMapFinalFramebuffer",
		m_FinalTexture->GetBackendTexture(), nullptr);
	ENSURE(m_FinalTextureFramebuffer);
}

void CMiniMapTexture::DestroyTextures()
{
	m_TerrainTexture.reset();
	m_FinalTexture.reset();
	m_TerrainData.reset();
}

void CMiniMapTexture::RebuildTerrainTexture(
	Renderer::Backend::IDeviceCommandContext* deviceCommandContext,
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
						if (!tex->GetTexture()->TryLoad())
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
		m_TerrainTexture.get(), Renderer::Backend::Format::R8G8B8A8_UNORM,
		m_TerrainData.get(), width * height * 4, 0, 0, width, height);
}

void CMiniMapTexture::RenderFinalTexture(
	Renderer::Backend::IDeviceCommandContext* deviceCommandContext,
	CLOSTexture& losTexture, CTerritoryTexture& territoryTexture)
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

	PROFILE3("Render minimap texture");
	GPU_SCOPED_LABEL(deviceCommandContext, "Render minimap texture");
	deviceCommandContext->SetFramebuffer(m_FinalTextureFramebuffer.get());

	const SViewPort oldViewPort = g_Renderer.GetViewport();
	const SViewPort viewPort = { 0, 0, FINAL_TEXTURE_SIZE, FINAL_TEXTURE_SIZE };
	g_Renderer.SetViewport(viewPort);

	CmpPtr<ICmpRangeManager> cmpRangeManager(m_Simulation, SYSTEM_ENTITY);
	ENSURE(cmpRangeManager);

	const float invTileMapSize = 1.0f / static_cast<float>(TERRAIN_TILE_SIZE * m_MapSize);
	const float texCoordMax = m_TerrainTexture ? static_cast<float>(m_MapSize - 1) / m_TerrainTexture->GetWidth() : 1.0f;

	Renderer::Backend::IShaderProgram* shader = nullptr;
	CShaderTechniquePtr tech;

	CShaderDefines baseDefines;
	baseDefines.Add(str_MINIMAP_BASE, str_1);

	tech = g_Renderer.GetShaderManager().LoadEffect(str_minimap, baseDefines);
	Renderer::Backend::GraphicsPipelineStateDesc pipelineStateDesc =
		tech->GetGraphicsPipelineStateDesc();
	deviceCommandContext->SetGraphicsPipelineState(pipelineStateDesc);
	deviceCommandContext->BeginPass();
	shader = tech->GetShader();

	if (m_TerrainTexture)
	{
		deviceCommandContext->SetTexture(
			shader->GetBindingSlot(str_baseTex), m_TerrainTexture.get());
	}

	CMatrix3D baseTransform;
	baseTransform.SetIdentity();
	CMatrix3D baseTextureTransform;
	baseTextureTransform.SetIdentity();

	CMatrix3D terrainTransform;
	terrainTransform.SetIdentity();
	terrainTransform.Scale(texCoordMax, texCoordMax, 1.0f);

	deviceCommandContext->SetUniform(
		shader->GetBindingSlot(str_transform),
		baseTransform._11, baseTransform._21, baseTransform._12, baseTransform._22);
	deviceCommandContext->SetUniform(
		shader->GetBindingSlot(str_textureTransform),
		terrainTransform._11, terrainTransform._21, terrainTransform._12, terrainTransform._22);
	deviceCommandContext->SetUniform(
		shader->GetBindingSlot(str_translation),
		baseTransform._14, baseTransform._24, terrainTransform._14, terrainTransform._24);

	if (m_TerrainTexture)
		DrawTexture(deviceCommandContext);
	deviceCommandContext->EndPass();

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
	deviceCommandContext->BeginPass();

	// Draw territory boundaries
	deviceCommandContext->SetTexture(
		shader->GetBindingSlot(str_baseTex), territoryTexture.GetTexture());
	deviceCommandContext->SetUniform(
		shader->GetBindingSlot(str_transform),
		baseTransform._11, baseTransform._21, baseTransform._12, baseTransform._22);
	const CMatrix3D& territoryTransform = territoryTexture.GetMinimapTextureMatrix();
	deviceCommandContext->SetUniform(
		shader->GetBindingSlot(str_textureTransform),
		territoryTransform._11, territoryTransform._21, territoryTransform._12, territoryTransform._22);
	deviceCommandContext->SetUniform(
		shader->GetBindingSlot(str_translation),
		baseTransform._14, baseTransform._24, territoryTransform._14, territoryTransform._24);

	DrawTexture(deviceCommandContext);
	deviceCommandContext->EndPass();

	tech = g_Renderer.GetShaderManager().LoadEffect(str_minimap_los, CShaderDefines());
	deviceCommandContext->SetGraphicsPipelineState(
		tech->GetGraphicsPipelineStateDesc());
	deviceCommandContext->BeginPass();
	shader = tech->GetShader();

	deviceCommandContext->SetTexture(
		shader->GetBindingSlot(str_baseTex), losTexture.GetTexture());
	deviceCommandContext->SetUniform(
		shader->GetBindingSlot(str_transform),
		baseTransform._11, baseTransform._21, baseTransform._12, baseTransform._22);
	const CMatrix3D& losTransform = losTexture.GetMinimapTextureMatrix();
	deviceCommandContext->SetUniform(
		shader->GetBindingSlot(str_textureTransform),
		losTransform._11, losTransform._21, losTransform._12, losTransform._22);
	deviceCommandContext->SetUniform(
		shader->GetBindingSlot(str_translation),
		baseTransform._14, baseTransform._24, losTransform._14, losTransform._24);

	DrawTexture(deviceCommandContext);

	deviceCommandContext->EndPass();

	// We might scale entities properly in the vertex shader but it requires
	// additional space in the vertex buffer. So we assume that we don't need
	// to change an entity size so often.
	// Radius with instancing is lower because an entity has a more round shape.
	const float entityRadius = static_cast<float>(m_MapSize) / 128.0f * (m_UseInstancing ? 5.0 : 6.0f);

	if (doUpdate)
	{
		m_Icons.clear();
		m_IconsCache.clear();

		CSimulation2::InterfaceList ents = m_Simulation.GetEntitiesWithInterface(IID_Minimap);

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

		bool iconsEnabled = false;
		CFG_GET_VAL("gui.session.minimap.icons.enabled", iconsEnabled);
		float iconsOpacity = 1.0f;
		CFG_GET_VAL("gui.session.minimap.icons.opacity", iconsOpacity);
		float iconsSizeScale = 1.0f;
		CFG_GET_VAL("gui.session.minimap.icons.sizescale", iconsSizeScale);

		bool iconsCountOverflow = false;

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
					v.position.X = posX.ToFloat();
					v.position.Y = posZ.ToFloat();

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
						AddEntity(v, attrColor, attrPos, entityRadius, m_UseInstancing);
						++m_EntitiesDrawn;
					}

					if (!iconsEnabled || !cmpMinimap->HasIcon())
						continue;

					const CellIconKey key{
						cmpMinimap->GetIconPath(), v.r, v.g, v.b};
					const u16 gridX = Clamp<u16>(
						(v.position.X * invTileMapSize) * ICON_COMBINING_GRID_SIZE, 0, ICON_COMBINING_GRID_SIZE - 1);
					const u16 gridY = Clamp<u16>(
						(v.position.Y * invTileMapSize) * ICON_COMBINING_GRID_SIZE, 0, ICON_COMBINING_GRID_SIZE - 1);
					CellIcon icon{
						gridX, gridY, cmpMinimap->GetIconSize() * iconsSizeScale * 0.5f, v.position};
					if (m_IconsCache.find(key) == m_IconsCache.end() && m_IconsCache.size() >= MAX_UNIQUE_ICON_COUNT)
					{
						iconsCountOverflow = true;
					}
					else
					{
						m_IconsCache[key].emplace_back(std::move(icon));
					}
				}
			}
		}

		// We need to combine too close icons with the same path, we use a grid for
		// that. But to save some allocations and space we store only the current
		// row.
		struct Cell
		{
			u32 count;
			float maxHalfSize;
			CVector2D averagePosition;
		};
		std::array<Cell, ICON_COMBINING_GRID_SIZE> gridRow;
		for (auto& [key, icons] : m_IconsCache)
		{
			CTexturePtr texture = g_Renderer.GetTextureManager().CreateTexture(
				CTextureProperties(key.path));
			const CColor color(key.r / 255.0f, key.g / 255.0f, key.b / 255.0f, iconsOpacity);

			std::sort(icons.begin(), icons.end(),
				[](const CellIcon& lhs, const CellIcon& rhs) -> bool
				{
					if (lhs.gridY != rhs.gridY)
						return lhs.gridY < rhs.gridY;
					return lhs.gridX < rhs.gridX;
				});

			for (auto beginIt = icons.begin(); beginIt != icons.end();)
			{
				auto endIt = std::next(beginIt);
				while (endIt != icons.end() && beginIt->gridY == endIt->gridY)
					++endIt;
				gridRow.fill({0, 0.0f, {}});
				for (; beginIt != endIt; ++beginIt)
				{
					Cell& cell = gridRow[beginIt->gridX];
					const float previousPositionWeight = static_cast<float>(cell.count) / (cell.count + 1);
					cell.averagePosition = cell.averagePosition * previousPositionWeight + beginIt->worldPosition / static_cast<float>(cell.count + 1);
					cell.maxHalfSize = std::max(cell.maxHalfSize, beginIt->halfSize);
					++cell.count;
				}
				for (const Cell& cell : gridRow)
				{
					if (cell.count == 0)
						continue;

					if (m_Icons.size() < MAX_ICON_COUNT)
					{
						m_Icons.emplace_back(Icon{
							texture, color, cell.averagePosition, cell.maxHalfSize});
					}
					else
						iconsCountOverflow = true;
				}
			}
		}

		if (iconsCountOverflow)
			LOGWARNING("Too many minimap icons to draw.");

		// Add the pinged vertices at the end, so they are drawn on top
		for (const MinimapUnitVertex& vertex : pingingVertices)
		{
			AddEntity(vertex, attrColor, attrPos, entityRadius, m_UseInstancing);
			++m_EntitiesDrawn;
		}

		ENSURE(m_EntitiesDrawn < MAX_ENTITIES_DRAWN);

		if (!m_UseInstancing)
		{
			VertexArrayIterator<u16> index = m_IndexArray.GetIterator();
			for (size_t entityIndex = 0; entityIndex < m_EntitiesDrawn; ++entityIndex)
			{
				index[entityIndex * 6 + 0] = static_cast<u16>(entityIndex * 4 + 0);
				index[entityIndex * 6 + 1] = static_cast<u16>(entityIndex * 4 + 1);
				index[entityIndex * 6 + 2] = static_cast<u16>(entityIndex * 4 + 2);
				index[entityIndex * 6 + 3] = static_cast<u16>(entityIndex * 4 + 0);
				index[entityIndex * 6 + 4] = static_cast<u16>(entityIndex * 4 + 2);
				index[entityIndex * 6 + 5] = static_cast<u16>(entityIndex * 4 + 3);
			}

			m_IndexArray.Upload();
		}

		m_VertexArray.Upload();
	}

	m_VertexArray.PrepareForRendering();

	if (m_EntitiesDrawn > 0)
	{
		CShaderDefines pointDefines;
		pointDefines.Add(str_MINIMAP_POINT, str_1);
		if (m_UseInstancing)
			pointDefines.Add(str_USE_GPU_INSTANCING, str_1);
		tech = g_Renderer.GetShaderManager().LoadEffect(str_minimap, pointDefines);
		deviceCommandContext->SetGraphicsPipelineState(
			tech->GetGraphicsPipelineStateDesc());
		deviceCommandContext->BeginPass();
		shader = tech->GetShader();

		CMatrix3D unitMatrix;
		unitMatrix.SetIdentity();
		// Convert world space coordinates into [0, 2].
		const float unitScale = invTileMapSize;
		unitMatrix.Scale(unitScale * 2.0f, unitScale * 2.0f, 1.0f);
		// Offset the coordinates to [-1, 1].
		unitMatrix.Translate(CVector3D(-1.0f, -1.0f, 0.0f));
		deviceCommandContext->SetUniform(
			shader->GetBindingSlot(str_transform),
			unitMatrix._11, unitMatrix._21, unitMatrix._12, unitMatrix._22);
		deviceCommandContext->SetUniform(
			shader->GetBindingSlot(str_translation),
			unitMatrix._14, unitMatrix._24, 0.0f, 0.0f);

		Renderer::Backend::IDeviceCommandContext::Rect scissorRect;
		scissorRect.x = scissorRect.y = 1;
		scissorRect.width = scissorRect.height = FINAL_TEXTURE_SIZE - 2;
		deviceCommandContext->SetScissors(1, &scissorRect);

		m_VertexArray.UploadIfNeeded(deviceCommandContext);

		const uint32_t stride = m_VertexArray.GetStride();
		const uint32_t firstVertexOffset = m_VertexArray.GetOffset() * stride;

		if (m_UseInstancing)
		{
			deviceCommandContext->SetVertexAttributeFormat(
				Renderer::Backend::VertexAttributeStream::POSITION,
				m_InstanceAttributePosition.format, m_InstanceAttributePosition.offset,
				m_InstanceVertexArray.GetStride(),
				Renderer::Backend::VertexAttributeRate::PER_VERTEX, 0);

			deviceCommandContext->SetVertexAttributeFormat(
				Renderer::Backend::VertexAttributeStream::UV1,
				m_AttributePos.format, m_AttributePos.offset, stride,
				Renderer::Backend::VertexAttributeRate::PER_INSTANCE, 1);
			deviceCommandContext->SetVertexAttributeFormat(
				Renderer::Backend::VertexAttributeStream::COLOR,
				m_AttributeColor.format, m_AttributeColor.offset, stride,
				Renderer::Backend::VertexAttributeRate::PER_INSTANCE, 1);

			deviceCommandContext->SetVertexBuffer(
				0, m_InstanceVertexArray.GetBuffer(), m_InstanceVertexArray.GetOffset());
			deviceCommandContext->SetVertexBuffer(
				1, m_VertexArray.GetBuffer(), firstVertexOffset);

			deviceCommandContext->SetUniform(shader->GetBindingSlot(str_width), entityRadius);

			deviceCommandContext->DrawInstanced(0, m_InstanceVertexArray.GetNumberOfVertices(), 0, m_EntitiesDrawn);
		}
		else
		{
			m_IndexArray.UploadIfNeeded(deviceCommandContext);

			deviceCommandContext->SetVertexAttributeFormat(
				Renderer::Backend::VertexAttributeStream::POSITION,
				m_AttributePos.format, m_AttributePos.offset, stride,
				Renderer::Backend::VertexAttributeRate::PER_VERTEX, 0);
			deviceCommandContext->SetVertexAttributeFormat(
				Renderer::Backend::VertexAttributeStream::COLOR,
				m_AttributeColor.format, m_AttributeColor.offset, stride,
				Renderer::Backend::VertexAttributeRate::PER_VERTEX, 0);

			deviceCommandContext->SetVertexBuffer(
				0, m_VertexArray.GetBuffer(), firstVertexOffset);
			deviceCommandContext->SetIndexBuffer(m_IndexArray.GetBuffer());

			deviceCommandContext->DrawIndexed(m_IndexArray.GetOffset(), m_EntitiesDrawn * 6, 0);
		}

		g_Renderer.GetStats().m_DrawCalls++;

		deviceCommandContext->SetScissors(0, nullptr);

		deviceCommandContext->EndPass();
	}

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
