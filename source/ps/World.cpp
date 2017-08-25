/* Copyright (C) 2017 Wildfire Games.
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

/**
 * File        : World.cpp
 * Project     : engine
 * Description : Contains the CWorld Class implementation.
 *
 **/
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
#include "ps/LoaderThunks.h"
#include "ps/World.h"
#include "renderer/Renderer.h"
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
 * @param pGame CGame * pGame pointer to the container game object.
 **/
CWorld::CWorld(CGame *pGame):
	m_pGame(pGame),
	m_Terrain(new CTerrain()),
	m_UnitManager(new CUnitManager()),
	m_MapReader(new CMapReader)
{
}

/**
 * Initializes the game world with the attributes provided.
 **/
void CWorld::RegisterInit(const CStrW& mapFile, JSRuntime* rt, JS::HandleValue settings, int playerID)
{
	// Load the map, if one was specified
	if (mapFile.length())
	{
		VfsPath mapfilename = VfsPath(mapFile).ChangeExtension(L".pmp");

		try
		{
			CTriggerManager* pTriggerManager = NULL;
			m_MapReader->LoadMap(mapfilename, rt, settings, m_Terrain,
				CRenderer::IsInitialised() ? g_Renderer.GetWaterManager() : NULL,
				CRenderer::IsInitialised() ? g_Renderer.GetSkyManager() : NULL,
				&g_LightEnv, m_pGame->GetView(),
				m_pGame->GetView() ? m_pGame->GetView()->GetCinema() : NULL,
				pTriggerManager, CRenderer::IsInitialised() ? &g_Renderer.GetPostprocManager() : NULL,
				m_pGame->GetSimulation2(), &m_pGame->GetSimulation2()->GetSimContext(), playerID, false);
				// fails immediately, or registers for delay loading
			RegMemFun(this, &CWorld::DeleteMapReader, L"CWorld::DeleteMapReader", 5);
		}
		catch (PSERROR_File& err)
		{
			SAFE_DELETE(m_MapReader);
			LOGERROR("Failed to load map %s: %s", mapfilename.string8(), err.what());
			throw PSERROR_Game_World_MapLoadFailed("Failed to load map.\nCheck application log for details.");
		}
	}
}

void CWorld::RegisterInitRMS(const CStrW& scriptFile, JSRuntime* rt, JS::HandleValue settings, int playerID)
{
	// If scriptFile is empty, a blank map will be generated using settings (no RMS run)
	CTriggerManager* pTriggerManager = NULL;
	m_MapReader->LoadRandomMap(scriptFile, rt, settings, m_Terrain,
		CRenderer::IsInitialised() ? g_Renderer.GetWaterManager() : NULL,
		CRenderer::IsInitialised() ? g_Renderer.GetSkyManager() : NULL,
		&g_LightEnv, m_pGame->GetView(),
		m_pGame->GetView() ? m_pGame->GetView()->GetCinema() : NULL,
		pTriggerManager, CRenderer::IsInitialised() ? &g_Renderer.GetPostprocManager() : NULL,
		m_pGame->GetSimulation2(), playerID);
		// registers for delay loading
	RegMemFun(this, &CWorld::DeleteMapReader, L"CWorld::DeleteMapReader", 5);
}

int CWorld::DeleteMapReader()
{
	SAFE_DELETE(m_MapReader);
	return 0;
}

/**
 * Destructor.
 *
 **/
CWorld::~CWorld()
{
	delete m_Terrain;
	delete m_UnitManager;
	delete m_MapReader;
}
