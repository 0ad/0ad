#ifndef _ps_World_H
#define _ps_World_H

class CGame;
class CGameAttributes;

class CTerrain;
extern CTerrain* g_Terrain_ptr;
#define g_Terrain (*g_Terrain_ptr)

#include "UnitManager.h"

class CWorld
{
	CGame *m_pGame;
	
	// These both point to the respective g_* globals - the plan is to remove
	// the globals and move them into CWorld members as soon as all code has
	// been converted
	CTerrain *m_pTerrain;
	CUnitManager *m_pUnitManager;
public:
	inline CWorld(CGame *pGame):
		m_pGame(pGame),
		m_pTerrain(&g_Terrain),
		m_pUnitManager(&g_UnitMan)
	{}

	/*
		Initialize the World - load the map and all objects
	*/
	void Initialize(CGameAttributes *pGameAttributes);
	
	inline CTerrain *GetTerrain()
	{	return m_pTerrain; }
	inline CUnitManager *GetUnitManager()
	{	return m_pUnitManager; }
};

#include "Game.h"
ERROR_SUBGROUP(Game, World);
ERROR_TYPE(Game_World, MapLoadFailed);

#endif
