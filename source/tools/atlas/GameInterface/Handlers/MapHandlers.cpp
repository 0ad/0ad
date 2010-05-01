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

#include "precompiled.h"

#include "MessageHandler.h"
#include "../GameLoop.h"

#include "graphics/GameView.h"
#include "graphics/MapWriter.h"
#include "graphics/Patch.h"
#include "graphics/Terrain.h"
#include "graphics/TextureEntry.h"
#include "graphics/TextureManager.h"
#include "ps/Game.h"
#include "ps/GameAttributes.h"
#include "ps/Loader.h"
#include "ps/World.h"
#include "renderer/Renderer.h"
#include "simulation/LOSManager.h"
#include "simulation/Simulation.h"
#include "simulation2/Simulation2.h"
#include "simulation2/components/ICmpPlayer.h"
#include "simulation2/components/ICmpPlayerManager.h"
#include "simulation2/components/ICmpPosition.h"

namespace
{
	void InitGame(const CStrW& map)
	{
		if (g_Game)
		{
			delete g_Game;
			g_Game = NULL;
		}

		// Set attributes for the game:

		g_GameAttributes.m_MapFile = map;
		// Make all players locally controlled
		for (int i = 1; i < 8; ++i) 
			g_GameAttributes.GetSlot(i)->AssignLocal();

		// Make the whole world visible
		g_GameAttributes.m_LOSSetting = LOS_SETTING_ALL_VISIBLE;
		g_GameAttributes.m_FogOfWar = false;

		// Don't use screenshot mode, because we want working AI for the
		// simulation-testing. Outside that simulation-testing, we avoid having
		// the units move into attack mode by never calling CEntity::update.
		g_GameAttributes.m_ScreenshotMode = false;

		// Initialise the game:
		g_Game = new CGame();
	}

	void AddDefaultPlayers()
	{
		CmpPtr<ICmpPlayerManager> cmpPlayerMan(*g_Game->GetSimulation2(), SYSTEM_ENTITY);
		debug_assert(!cmpPlayerMan.null());

		// TODO: pick a sensible number, give them names and colours etc
		size_t numPlayers = 4;
		for (size_t i = 0; i < numPlayers; ++i)
		{
			entity_id_t ent = g_Game->GetSimulation2()->AddEntity(L"special/player");
			cmpPlayerMan->AddPlayer(ent);
		}
		// Also TODO: Maybe it'd be sensible to load this from a map XML file via CMapReader,
		// rather than duplicating the creation code here?
	}

	void StartGame()
	{
		PSRETURN ret = g_Game->StartGame(&g_GameAttributes);
		debug_assert(ret == PSRETURN_OK);
		LDR_NonprogressiveLoad();
		ret = g_Game->ReallyStartGame();
		debug_assert(ret == PSRETURN_OK);

		if (!g_UseSimulation2)
		{
			// Make sure entities get rendered in the correct location
			g_Game->GetSimulation()->Interpolate(0.0);
		}
	}
}

namespace AtlasMessage {

MESSAGEHANDLER(GenerateMap)
{
	InitGame(L"");

	// Convert size in patches to number of vertices
	int vertices = msg->size * PATCH_SIZE + 1;

	// Generate flat heightmap
	u16* heightmap = new u16[vertices*vertices];
	for (int z = 0; z < vertices; ++z)
		for (int x = 0; x < vertices; ++x)
			heightmap[x + z*vertices] = 16384;

	// Initialise terrain using the heightmap
	CTerrain* terrain = g_Game->GetWorld()->GetTerrain();
	terrain->Initialize(msg->size, heightmap);

	delete[] heightmap;

	if (g_UseSimulation2)
		AddDefaultPlayers();

	// Start the game, load data files - this must be done before initialising
	// the terrain texture below, since the terrains must be loaded before being
	// used.
	StartGame();

	// Cover terrain with default texture
	// TODO: split into fCoverWithTexture
	CTextureEntry* texentry = g_TexMan.FindTexture("grass1_spring"); // TODO: make default customisable
	Handle tex = texentry ? texentry->GetHandle() : 0;

	int patchesPerSide = terrain->GetPatchesPerSide();
	for (int pz = 0; pz < patchesPerSide; ++pz)
	{
		for (int px = 0; px < patchesPerSide; ++px)
		{
			CPatch* patch = terrain->GetPatch(px, pz);	// can't fail

			for (ssize_t z = 0; z < PATCH_SIZE; ++z)
			{
				for (ssize_t x = 0; x < PATCH_SIZE; ++x)
				{
					patch->m_MiniPatches[z][x].Tex1 = tex;
					patch->m_MiniPatches[z][x].Tex1Priority = 0;
				}
			}
		}
	}

}

MESSAGEHANDLER(LoadMap)
{
	InitGame(*msg->filename);
	StartGame();
}

MESSAGEHANDLER(SaveMap)
{
	CMapWriter writer;
	const VfsPath pathname = VfsPath(L"maps/scenarios/") / *msg->filename;
	writer.SaveMap(pathname,
		g_Game->GetWorld()->GetTerrain(), &g_Game->GetWorld()->GetUnitManager(),
		g_Renderer.GetWaterManager(), g_Renderer.GetSkyManager(),
		&g_LightEnv, g_Game->GetView()->GetCamera(), g_Game->GetView()->GetCinema(),
		g_Game->GetSimulation2());
}

}
