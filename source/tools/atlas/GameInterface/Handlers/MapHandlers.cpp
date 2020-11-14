/* Copyright (C) 2020 Wildfire Games.
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

#include "MessageHandler.h"
#include "../CommandProc.h"
#include "../GameLoop.h"
#include "../MessagePasser.h"

#include "graphics/GameView.h"
#include "graphics/LOSTexture.h"
#include "graphics/MapIO.h"
#include "graphics/MapWriter.h"
#include "graphics/Patch.h"
#include "graphics/Terrain.h"
#include "graphics/TerrainTextureEntry.h"
#include "graphics/TerrainTextureManager.h"
#include "gui/ObjectTypes/CMiniMap.h"
#include "lib/bits.h"
#include "lib/file/vfs/vfs_path.h"
#include "lib/status.h"
#include "maths/MathUtil.h"
#include "ps/CLogger.h"
#include "ps/Filesystem.h"
#include "ps/Game.h"
#include "ps/GameSetup/GameSetup.h"
#include "ps/Loader.h"
#include "ps/World.h"
#include "renderer/Renderer.h"
#include "renderer/WaterManager.h"
#include "scriptinterface/ScriptInterface.h"
#include "simulation2/Simulation2.h"
#include "simulation2/components/ICmpOwnership.h"
#include "simulation2/components/ICmpPlayer.h"
#include "simulation2/components/ICmpPlayerManager.h"
#include "simulation2/components/ICmpPosition.h"
#include "simulation2/components/ICmpRangeManager.h"
#include "simulation2/components/ICmpTemplateManager.h"
#include "simulation2/components/ICmpTerrain.h"
#include "simulation2/components/ICmpVisual.h"
#include "simulation2/system/ParamNode.h"

namespace
{
	void InitGame()
	{
		if (g_Game)
		{
			delete g_Game;
			g_Game = NULL;
		}

		g_Game = new CGame(false);

		// Default to player 1 for playtesting
		g_Game->SetPlayerID(1);
	}

	void StartGame(JS::MutableHandleValue attrs)
	{
		g_Game->StartGame(attrs, "");

		// TODO: Non progressive load can fail - need a decent way to handle this
		LDR_NonprogressiveLoad();

		// Disable fog-of-war - this must be done before starting the game,
		// as visual actors cache their visibility state on first render.
		CmpPtr<ICmpRangeManager> cmpRangeManager(*g_Game->GetSimulation2(), SYSTEM_ENTITY);
		if (cmpRangeManager)
			cmpRangeManager->SetLosRevealAll(-1, true);

		PSRETURN ret = g_Game->ReallyStartGame();
		ENSURE(ret == PSRETURN_OK);
	}
}

namespace AtlasMessage {

QUERYHANDLER(GenerateMap)
{
	try
	{
		InitGame();

		// Random map
		const ScriptInterface& scriptInterface = g_Game->GetSimulation2()->GetScriptInterface();
		ScriptInterface::Request rq(scriptInterface);

		JS::RootedValue settings(rq.cx);
		scriptInterface.ParseJSON(*msg->settings, &settings);
		scriptInterface.SetProperty(settings, "mapType", "random");

		JS::RootedValue attrs(rq.cx);
		ScriptInterface::CreateObject(
			rq,
			&attrs,
			"mapType", "random",
			"script", *msg->filename,
			"settings", settings);

		StartGame(&attrs);

		msg->status = 0;
	}
	catch (PSERROR_Game_World_MapLoadFailed&)
	{
		// Cancel loading
		LDR_Cancel();

		// Since map generation failed and we don't know why, use the blank map as a fallback

		InitGame();

		const ScriptInterface& scriptInterface = g_Game->GetSimulation2()->GetScriptInterface();
		ScriptInterface::Request rq(scriptInterface);

		// Set up 8-element array of empty objects to satisfy init
		JS::RootedValue playerData(rq.cx);
		ScriptInterface::CreateArray(rq, &playerData);

		for (int i = 0; i < 8; ++i)
		{
			JS::RootedValue player(rq.cx);
			ScriptInterface::CreateObject(rq, &player);
			scriptInterface.SetPropertyInt(playerData, i, player);
		}

		JS::RootedValue settings(rq.cx);
		ScriptInterface::CreateObject(
			rq,
			&settings,
			"mapType", "scenario",
			"PlayerData", playerData);

		JS::RootedValue attrs(rq.cx);
		ScriptInterface::CreateObject(
			rq,
			&attrs,
			"mapType", "scenario",
			"map", "maps/scenarios/_default",
			"settings", settings);

		StartGame(&attrs);

		msg->status = -1;
	}
}

MESSAGEHANDLER(LoadMap)
{
	InitGame();

	const ScriptInterface& scriptInterface = g_Game->GetSimulation2()->GetScriptInterface();
	ScriptInterface::Request rq(scriptInterface);

	// Scenario
	CStrW map = *msg->filename;
	CStrW mapBase = map.BeforeLast(L".pmp"); // strip the file extension, if any

	JS::RootedValue attrs(rq.cx);

	ScriptInterface::CreateObject(
		rq,
		&attrs,
		"mapType", "scenario",
		"map", mapBase);

	StartGame(&attrs);
}

MESSAGEHANDLER(ImportHeightmap)
{
	std::vector<u16> heightmap_source;
	if (LoadHeightmapImageOs(*msg->filename, heightmap_source) != INFO::OK)
	{
		LOGERROR("Failed to decode heightmap.");
		return;
	}

	// resize terrain to heightmap size
	// Notice that the number of tiles/pixels per side of the heightmap image is
	// one less than the number of vertices per side of the heightmap.
	CTerrain* terrain = g_Game->GetWorld()->GetTerrain();
	const ssize_t newSize = (sqrt(heightmap_source.size()) - 1) / PATCH_SIZE;
	const ssize_t offset = (newSize - terrain->GetPatchesPerSide()) / 2;
	terrain->ResizeAndOffset(newSize, offset, offset);

	// copy heightmap data into map
	u16* heightmap = g_Game->GetWorld()->GetTerrain()->GetHeightMap();
	ENSURE(heightmap_source.size() == (std::size_t) SQR(g_Game->GetWorld()->GetTerrain()->GetVerticesPerSide()));
	std::copy(heightmap_source.begin(), heightmap_source.end(), heightmap);

	// update simulation
	CmpPtr<ICmpTerrain> cmpTerrain(*g_Game->GetSimulation2(), SYSTEM_ENTITY);
	if (cmpTerrain)
		cmpTerrain->ReloadTerrain();

	g_Game->GetView()->GetLOSTexture().MakeDirty();
}

MESSAGEHANDLER(SaveMap)
{
	CMapWriter writer;
	VfsPath pathname = VfsPath(*msg->filename).ChangeExtension(L".pmp");
	writer.SaveMap(pathname,
		g_Game->GetWorld()->GetTerrain(),
		g_Renderer.GetWaterManager(), g_Renderer.GetSkyManager(),
		&g_LightEnv, g_Game->GetView()->GetCamera(), g_Game->GetView()->GetCinema(),
		&g_Renderer.GetPostprocManager(),
		g_Game->GetSimulation2());
}

QUERYHANDLER(GetMapSettings)
{
	msg->settings = g_Game->GetSimulation2()->GetMapSettingsString();
}

BEGIN_COMMAND(SetMapSettings)
{
	std::string m_OldSettings, m_NewSettings;

	void SetSettings(const std::string& settings)
	{
		g_Game->GetSimulation2()->SetMapSettings(settings);
	}

	void Do()
	{
		m_OldSettings = g_Game->GetSimulation2()->GetMapSettingsString();
		m_NewSettings = *msg->settings;

		SetSettings(m_NewSettings);
	}

	// TODO: we need some way to notify the Atlas UI when the settings are changed
	//	externally, otherwise this will have no visible effect
	void Undo()
	{
		// SetSettings(m_OldSettings);
	}

	void Redo()
	{
		// SetSettings(m_NewSettings);
	}

	void MergeIntoPrevious(cSetMapSettings* prev)
	{
		prev->m_NewSettings = m_NewSettings;
	}
};
END_COMMAND(SetMapSettings)

MESSAGEHANDLER(LoadPlayerSettings)
{
	g_Game->GetSimulation2()->LoadPlayerSettings(msg->newplayers);
}

QUERYHANDLER(GetMapSizes)
{
	msg->sizes = g_Game->GetSimulation2()->GetMapSizes();
}

QUERYHANDLER(RasterizeMinimap)
{
	// TODO: remove the code duplication of the rasterization algorithm, using
	// CMinimap version.
	const CTerrain* terrain = g_Game->GetWorld()->GetTerrain();
	const ssize_t dimension = terrain->GetVerticesPerSide() - 1;
	const ssize_t bpp = 24;
	const ssize_t imageDataSize = dimension * dimension * (bpp / 8);

	std::vector<u8> imageBytes(imageDataSize);

	float shallowPassageHeight = CMiniMap::GetShallowPassageHeight();

	ssize_t w = dimension;
	ssize_t h = dimension;
	float waterHeight = g_Renderer.GetWaterManager()->m_WaterHeight;

	for (ssize_t j = 0; j < h; ++j)
	{
		// Work backwards to vertically flip the image.
		ssize_t position = 3 * (h - j - 1) * dimension;
		for (ssize_t i = 0; i < w; ++i)
		{
			float avgHeight = (terrain->GetVertexGroundLevel(i, j)
				+ terrain->GetVertexGroundLevel(i + 1, j)
				+ terrain->GetVertexGroundLevel(i, j + 1)
				+ terrain->GetVertexGroundLevel(i + 1, j + 1)
				) / 4.0f;

			if (avgHeight < waterHeight && avgHeight > waterHeight - shallowPassageHeight)
			{
				// shallow water
				imageBytes[position++] = 0x70;
				imageBytes[position++] = 0x98;
				imageBytes[position++] = 0xc0;
			}
			else if (avgHeight < waterHeight)
			{
				// Set water as constant color for consistency on different maps
				imageBytes[position++] = 0x50;
				imageBytes[position++] = 0x78;
				imageBytes[position++] = 0xa0;
			}
			else
			{
				u32 color = std::numeric_limits<u32>::max();
				u32 hmap = static_cast<u32>(terrain->GetHeightMap()[j * dimension + i]) >> 8;
				float scale = hmap / 3.0f + 170.0f / 255.0f;

				CMiniPatch* mp = terrain->GetTile(i, j);
				if (mp)
				{
					CTerrainTextureEntry* tex = mp->GetTextureEntry();
					if (tex)
						color = tex->GetBaseColor();
				}

				// Convert
				imageBytes[position++] = static_cast<u8>(static_cast<float>(color & 0xff) * scale);
				imageBytes[position++] = static_cast<u8>(static_cast<float>((color >> 8) & 0xff) * scale);
				imageBytes[position++] = static_cast<u8>(static_cast<float>((color >> 16) & 0xff) * scale);
			}
		}
	}

	msg->imageBytes = std::move(imageBytes);
	msg->dimension = dimension;
}

QUERYHANDLER(GetRMSData)
{
	msg->data = g_Game->GetSimulation2()->GetRMSData();
}

QUERYHANDLER(GetCurrentMapSize)
{
	msg->size = g_Game->GetWorld()->GetTerrain()->GetTilesPerSide();
}

BEGIN_COMMAND(ResizeMap)
{
	bool Within(const CFixedVector3D& pos, const int centerX, const int centerZ, const int radius)
	{
		int dx = abs(pos.X.ToInt_RoundToZero() - centerX);
		if (dx > radius)
			return false;
		int dz = abs(pos.Z.ToInt_RoundToZero() - centerZ);
		if (dz > radius)
			return false;
		if (dx + dz <= radius)
			return true;
		return dx * dx + dz * dz <= radius * radius;
	}

	struct DeletedObject
	{
		entity_id_t entityId;
		CStr templateName;
		player_id_t owner;
		CFixedVector3D pos;
		CFixedVector3D rot;
		u32 actorSeed;
	};

	ssize_t m_OldPatches, m_NewPatches;
	int m_OffsetX, m_OffsetY;

	u16* m_Heightmap;
	CPatch*	m_Patches;

	std::vector<DeletedObject> m_DeletedObjects;
	std::vector<std::pair<entity_id_t, CFixedVector3D>> m_OldPositions;
	std::vector<std::pair<entity_id_t, CFixedVector3D>> m_NewPositions;

	cResizeMap() : m_Heightmap(nullptr), m_Patches(nullptr)
	{
	}

	~cResizeMap()
	{
		delete[] m_Heightmap;
		delete[] m_Patches;
	}

	void MakeDirty()
	{
		CmpPtr<ICmpTerrain> cmpTerrain(*g_Game->GetSimulation2(), SYSTEM_ENTITY);
		if (cmpTerrain)
			cmpTerrain->ReloadTerrain();

		// The LOS texture won't normally get updated when running Atlas
		// (since there's no simulation updates), so explicitly dirty it
		g_Game->GetView()->GetLOSTexture().MakeDirty();
	}

	void ResizeTerrain(ssize_t patches, int offsetX, int offsetY)
	{
		CTerrain* terrain = g_Game->GetWorld()->GetTerrain();
		terrain->ResizeAndOffset(patches, -offsetX, -offsetY);
	}

	void DeleteObjects(const std::vector<DeletedObject>& deletedObjects)
	{
		for (const DeletedObject& deleted : deletedObjects)
			g_Game->GetSimulation2()->DestroyEntity(deleted.entityId);

		g_Game->GetSimulation2()->FlushDestroyedEntities();
	}

	void RestoreObjects(const std::vector<DeletedObject>& deletedObjects)
	{
		CSimulation2& sim = *g_Game->GetSimulation2();

		for (const DeletedObject& deleted : deletedObjects)
		{
			entity_id_t ent = sim.AddEntity(deleted.templateName.FromUTF8(), deleted.entityId);
			if (ent == INVALID_ENTITY)
			{
				LOGERROR("Failed to load entity template '%s'", deleted.templateName.c_str());
			}
			else
			{
				CmpPtr<ICmpPosition> cmpPosition(sim, deleted.entityId);
				if (cmpPosition)
				{
					cmpPosition->JumpTo(deleted.pos.X, deleted.pos.Z);
					cmpPosition->SetXZRotation(deleted.rot.X, deleted.rot.Z);
					cmpPosition->SetYRotation(deleted.rot.Y);
				}

				CmpPtr<ICmpOwnership> cmpOwnership(sim, deleted.entityId);
				if (cmpOwnership)
					cmpOwnership->SetOwner(deleted.owner);

				CmpPtr<ICmpVisual> cmpVisual(sim, deleted.entityId);
				if (cmpVisual)
					cmpVisual->SetActorSeed(deleted.actorSeed);
			}
		}
	}

	void SetMovedEntitiesPosition(const std::vector<std::pair<entity_id_t, CFixedVector3D>>& movedObjects)
	{
		for (const std::pair<entity_id_t, CFixedVector3D>& obj : movedObjects)
		{
			const entity_id_t id = obj.first;
			const CFixedVector3D position = obj.second;
			CmpPtr<ICmpPosition> cmpPosition(*g_Game->GetSimulation2(), id);
			ENSURE(cmpPosition);
			cmpPosition->JumpTo(position.X, position.Z);
		}
	}

	void Do()
	{
		CSimulation2& sim = *g_Game->GetSimulation2();
		CmpPtr<ICmpTemplateManager> cmpTemplateManager(sim, SYSTEM_ENTITY);
		ENSURE(cmpTemplateManager);

		CmpPtr<ICmpTerrain> cmpTerrain(sim, SYSTEM_ENTITY);
		if (!cmpTerrain)
		{
			m_OldPatches = m_NewPatches = 0;
			m_OffsetX = m_OffsetY = 0;
		}
		else
		{
			m_OldPatches = static_cast<ssize_t>(cmpTerrain->GetTilesPerSide() / PATCH_SIZE);
			m_NewPatches = msg->tiles / PATCH_SIZE;
			m_OffsetX = msg->offsetX / PATCH_SIZE;
			// Need to flip direction of vertical offset, due to screen mapping order.
			m_OffsetY = -(msg->offsetY / PATCH_SIZE);

			CTerrain* terrain = cmpTerrain->GetCTerrain();
			m_Heightmap = new u16[(m_OldPatches * PATCH_SIZE + 1) * (m_OldPatches * PATCH_SIZE + 1)];
			std::copy_n(terrain->GetHeightMap(), (m_OldPatches * PATCH_SIZE + 1) * (m_OldPatches * PATCH_SIZE + 1), m_Heightmap);
			m_Patches = new CPatch[m_OldPatches * m_OldPatches];
			for (ssize_t j = 0; j < m_OldPatches; ++j)
				for (ssize_t i = 0; i < m_OldPatches; ++i)
				{
					CPatch& src = *(terrain->GetPatch(i, j));
					CPatch& dst = m_Patches[j * m_OldPatches + i];
					std::copy_n(&src.m_MiniPatches[0][0], PATCH_SIZE * PATCH_SIZE, &dst.m_MiniPatches[0][0]);
				}
		}

		const int radiusInTerrainUnits = m_NewPatches * PATCH_SIZE * TERRAIN_TILE_SIZE / 2 * (1.f - 1e-6f);
		// Opposite direction offset, as we move the destination onto the source, not the source into the destination.
		const int mapCenterX = (m_OldPatches / 2 - m_OffsetX) * PATCH_SIZE * TERRAIN_TILE_SIZE;
		const int mapCenterZ = (m_OldPatches / 2 - m_OffsetY) * PATCH_SIZE * TERRAIN_TILE_SIZE;
		// The offset to move units by is opposite the direction the map is moved, and from the corner.
		const int offsetX = ((m_NewPatches - m_OldPatches) / 2 + m_OffsetX) * PATCH_SIZE * TERRAIN_TILE_SIZE;
		const int offsetZ = ((m_NewPatches - m_OldPatches) / 2 + m_OffsetY) * PATCH_SIZE * TERRAIN_TILE_SIZE;
		const CFixedVector3D offset = CFixedVector3D(fixed::FromInt(offsetX), fixed::FromInt(0), fixed::FromInt(offsetZ));

		const CSimulation2::InterfaceListUnordered& ents = sim.GetEntitiesWithInterfaceUnordered(IID_Selectable);
		for (const std::pair<entity_id_t, IComponent*>& ent : ents)
		{
			const entity_id_t entityId = ent.first;

			CmpPtr<ICmpPosition> cmpPosition(sim, entityId);

			if (cmpPosition && cmpPosition->IsInWorld() && Within(cmpPosition->GetPosition(), mapCenterX, mapCenterZ, radiusInTerrainUnits))
			{
				CFixedVector3D position = cmpPosition->GetPosition();

				m_NewPositions.emplace_back(entityId, position + offset);
				m_OldPositions.emplace_back(entityId, position);
			}
			else
			{
				DeletedObject deleted;
				deleted.entityId = entityId;
				deleted.templateName = cmpTemplateManager->GetCurrentTemplateName(entityId);

				// If the entity has a position, but the ending position is not valid;
				if (cmpPosition)
				{
					deleted.pos = cmpPosition->GetPosition();
					deleted.rot = cmpPosition->GetRotation();
				}

				CmpPtr<ICmpOwnership> cmpOwnership(sim, entityId);
				if (cmpOwnership)
					deleted.owner = cmpOwnership->GetOwner();

				CmpPtr<ICmpVisual> cmpVisual(sim, deleted.entityId);
				if (cmpVisual)
					deleted.actorSeed = cmpVisual->GetActorSeed();

				m_DeletedObjects.push_back(deleted);
			}
		}

		DeleteObjects(m_DeletedObjects);
		ResizeTerrain(m_NewPatches, m_OffsetX, m_OffsetY);
		SetMovedEntitiesPosition(m_NewPositions);
		MakeDirty();
	}

	void Undo()
	{
		if (m_Heightmap == nullptr || m_Patches == nullptr)
		{
			// If there previously was no data, just resize to old (probably not originally valid).
			ResizeTerrain(m_OldPatches, -m_OffsetX, -m_OffsetY);
		}
		else
		{
			CSimulation2& sim = *g_Game->GetSimulation2();
			CmpPtr<ICmpTerrain> cmpTerrain(sim, SYSTEM_ENTITY);
			CTerrain* terrain = cmpTerrain->GetCTerrain();

			terrain->Initialize(m_OldPatches, m_Heightmap);
			// Copy terrain data back.
			for (ssize_t j = 0; j < m_OldPatches; ++j)
				for (ssize_t i = 0; i < m_OldPatches; ++i)
				{
					CPatch& src = m_Patches[j * m_OldPatches + i];
					CPatch& dst = *(terrain->GetPatch(i, j));
					std::copy_n(&src.m_MiniPatches[0][0], PATCH_SIZE * PATCH_SIZE, &dst.m_MiniPatches[0][0]);
				}
		}
		RestoreObjects(m_DeletedObjects);
		SetMovedEntitiesPosition(m_OldPositions);
		MakeDirty();
	}

	void Redo()
	{
		DeleteObjects(m_DeletedObjects);
		ResizeTerrain(m_NewPatches, m_OffsetX, m_OffsetY);
		SetMovedEntitiesPosition(m_NewPositions);
		MakeDirty();
	}
};
END_COMMAND(ResizeMap)

QUERYHANDLER(VFSFileExists)
{
	msg->exists = VfsFileExists(*msg->path);
}

QUERYHANDLER(VFSFileRealPath)
{
	VfsPath pathname(*msg->path);
	if (pathname.empty())
		return;
	OsPath realPathname;
	if (g_VFS->GetRealPath(pathname, realPathname) == INFO::OK)
		msg->realPath = realPathname.string();
}

static Status AddToFilenames(const VfsPath& pathname, const CFileInfo& UNUSED(fileInfo), const uintptr_t cbData)
{
	std::vector<std::wstring>& filenames = *(std::vector<std::wstring>*)cbData;
	filenames.push_back(pathname.string().c_str());
	return INFO::OK;
}

QUERYHANDLER(GetMapList)
{
#define GET_FILE_LIST(path, list) \
	std::vector<std::wstring> list; \
	vfs::ForEachFile(g_VFS, path, AddToFilenames, (uintptr_t)&list, L"*.xml", vfs::DIR_RECURSIVE); \
	msg->list = list;

	GET_FILE_LIST(L"maps/scenarios/", scenarioFilenames);
	GET_FILE_LIST(L"maps/skirmishes/", skirmishFilenames);
	GET_FILE_LIST(L"maps/tutorials/", tutorialFilenames);
#undef GET_FILE_LIST
}

QUERYHANDLER(GetVictoryConditionData)
{
	msg->data = g_Game->GetSimulation2()->GetVictoryConditiondData();
}

}
