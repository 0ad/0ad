#ifndef _ps_World_H
#define _ps_World_H

#include "Terrain.h"
#include "UnitManager.h"

class CGame;
class CGameAttributes;

class CWorld
{
	CGame *m_pGame;
	
	// These both point to the respective g_* globals - the plan is to remove
	// the globals and move them into CWorld members as soon as all code has
	// been converted
	CTerrain m_Terrain;
	CUnitManager &m_UnitManager;
public:
	inline CWorld(CGame *pGame):
		m_pGame(pGame),
		m_Terrain(),
		m_UnitManager(g_UnitMan)
	{}

	/*
		Initialize the World - load the map and all objects
	*/
	void Initialize(CGameAttributes *pGameAttributes);
	
	inline CTerrain *GetTerrain()
	{	return &m_Terrain; }
	inline CUnitManager *GetUnitManager()
	{	return &m_UnitManager; }
};

#include "Game.h"
ERROR_SUBGROUP(Game, World);
ERROR_TYPE(Game_World, MapLoadFailed);

#endif
