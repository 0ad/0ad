#ifndef _ps_World_H
#define _ps_World_H

#include "Terrain.h"
#include "UnitManager.h"
#include "EntityManager.h"

class CGame;
class CGameAttributes;

class CWorld
{
	CGame *m_pGame;
	
	CTerrain m_Terrain;
	// These both point to the respective g_* globals - the plan is to remove
	// the globals and move them into CWorld members as soon as all code has
	// been converted
	CUnitManager &m_UnitManager;
	CEntityManager &m_EntityManager;
public:
	inline CWorld(CGame *pGame):
		m_pGame(pGame),
		m_Terrain(),
		m_UnitManager(g_UnitMan),
		m_EntityManager(*new CEntityManager())
	{}

	~CWorld();

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
