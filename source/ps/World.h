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

/**
 * File        : World.h
 * Project     : engine
 * Description : Contains the CWorld Class which contains all the entities and represents them at a specific moment in time.
 *
 **/
#ifndef INCLUDED_WORLD
#define INCLUDED_WORLD

#include "ps/CStrForward.h"
#include "ps/Errors.h"

#include <memory>

#ifndef ERROR_GROUP_GAME_DEFINED
#define ERROR_GROUP_GAME_DEFINED
ERROR_GROUP(Game);
#endif
ERROR_SUBGROUP(Game, World);
ERROR_TYPE(Game_World, MapLoadFailed);

class CGame;
class CUnitManager;
class CTerrain;
class CMapReader;
class ScriptContext;

/**
 * CWorld is a general data class containing whatever is needed to accurately represent the world.
 * This includes the map, entities, influence maps, tiles, heightmap, etc.
 **/
class CWorld
{
public:
	CWorld(CGame& game);
	~CWorld();

	/*
	Initialize the World - load the map and all objects
	*/
	void RegisterInit(const CStrW& mapFile, const ScriptContext& cx, JS::HandleValue settings, int playerID);

	/*
	Initialize the World - generate and load the random map
	*/
	void RegisterInitRMS(const CStrW& scriptFile, const ScriptContext& cx, JS::HandleValue settings, int playerID);

	/**
	 * Explicitly delete m_MapReader once the map has finished loading.
	 **/
	int DeleteMapReader();

	/**
	 * Get a reference to the terrain object.
	 *
	 * @return CTerrain& dereferenced m_Terrain.
	 **/
	CTerrain& GetTerrain()
	{
		return *m_Terrain;
	}

	/**
	 * Get a reference to the unit manager object.
	 *
	 * @return CUnitManager& dereferenced m_UnitManager.
	 **/
	CUnitManager& GetUnitManager()
	{
		return *m_UnitManager;
	}

private:
	/**
	 * Reference to the CGame object representing the game.
	 */
	CGame& m_Game;

	/**
	 * The CTerrain object represents the height map.
	 */
	const std::unique_ptr<CTerrain> m_Terrain;

	/**
	 * The CUnitManager that holds all the units in the world.
	 */
	const std::unique_ptr<CUnitManager> m_UnitManager;

	/**
	 * The map reader gets deleted just after the map is read.
	 */
	std::unique_ptr<CMapReader> m_MapReader;
};

// rationale: see definition.
class CLightEnv;
extern CLightEnv g_LightEnv;

#endif
