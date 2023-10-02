/* Copyright (C) 2023 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "precompiled.h"

#include "graphics/GameView.h"
#include "graphics/LightEnv.h"
#include "graphics/MapReader.h"
#include "graphics/MapWriter.h"
#include "graphics/Terrain.h"
#include "graphics/Terrain.h"
#include "graphics/UnitManager.h"
#include "lib/timer.h"
#include "ps/CLogger.h"
#include "ps/CStr.h"
#include "ps/Errors.h"
#include "ps/Game.h"
#include "ps/Loader.h"
#include "ps/World.h"
#include "renderer/Renderer.h"
#include "renderer/SceneRenderer.h"
#include "simulation2/Simulation2.h"

/**
 * Global light settings.
 * It is not a member of CWorld because it is passed
 * to the renderer before CWorld exists.
 **/
CLightEnv g_LightEnv;


/**
 * Constructor.
 *
 * @param game CGame& game reference to the container game object.
 **/
CWorld::CWorld(CGame& game):
	m_Game{game},
	m_Terrain{std::make_unique<CTerrain>()},
	m_UnitManager{std::make_unique<CUnitManager>()},
	m_MapReader{std::make_unique<CMapReader>()}
{
}

CWorld::~CWorld() = default;

/**
 * Initializes the game world with the attributes provided.
 **/
void CWorld::RegisterInit(const CStrW& mapFile, const ScriptContext& cx, JS::HandleValue settings, int playerID)
{
	// Load the map, if one was specified
	if (mapFile.length())
	{
		VfsPath mapfilename = VfsPath(mapFile).ChangeExtension(L".pmp");

		try
		{
			CTriggerManager* pTriggerManager = nullptr;
			m_MapReader->LoadMap(mapfilename, cx, settings, m_Terrain.get(),
				CRenderer::IsInitialised() ? &g_Renderer.GetSceneRenderer().GetWaterManager() :
					nullptr,
				CRenderer::IsInitialised() ? &g_Renderer.GetSceneRenderer().GetSkyManager() :
					nullptr, &g_LightEnv, m_Game.GetView(),
				m_Game.GetView() ? m_Game.GetView()->GetCinema() : nullptr, pTriggerManager,
				CRenderer::IsInitialised() ? &g_Renderer.GetPostprocManager() : nullptr,
				m_Game.GetSimulation2(), &m_Game.GetSimulation2()->GetSimContext(), playerID,
				false);
				// fails immediately, or registers for delay loading
			LDR_Register([this](const double)
			{
				return DeleteMapReader();
			}, L"CWorld::DeleteMapReader", 5);
		}
		catch (PSERROR_File& err)
		{
			m_MapReader.reset();
			LOGERROR("Failed to load map %s: %s", mapfilename.string8(), err.what());
			throw PSERROR_Game_World_MapLoadFailed("Failed to load map.\nCheck application log for details.");
		}
	}
}

void CWorld::RegisterInitRMS(const CStrW& scriptFile, const ScriptContext& cx, JS::HandleValue settings, int playerID)
{
	// If scriptFile is empty, a blank map will be generated using settings (no RMS run)
	CTriggerManager* pTriggerManager = nullptr;
	m_MapReader->LoadRandomMap(scriptFile, cx, settings, m_Terrain.get(),
		CRenderer::IsInitialised() ? &g_Renderer.GetSceneRenderer().GetWaterManager() : nullptr,
		CRenderer::IsInitialised() ? &g_Renderer.GetSceneRenderer().GetSkyManager() : nullptr,
		&g_LightEnv, m_Game.GetView(), m_Game.GetView() ? m_Game.GetView()->GetCinema() : nullptr,
		pTriggerManager, CRenderer::IsInitialised() ? &g_Renderer.GetPostprocManager() : nullptr,
		m_Game.GetSimulation2(), playerID);
		// registers for delay loading
	LDR_Register([this](const double)
	{
		return DeleteMapReader();
	}, L"CWorld::DeleteMapReader", 5);
}

int CWorld::DeleteMapReader()
{
	m_MapReader.reset();
	return 0;
}
