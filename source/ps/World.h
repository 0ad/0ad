/**
 * File        : World.h
 * Project     : engine
 * Description : Contains the CWorld Class which contains all the entities and represents them at a specific moment in time.
 *
 **/
#ifndef INCLUDED_WORLD
#define INCLUDED_WORLD

#include "ps/Errors.h"

#ifndef ERROR_GROUP_GAME_DEFINED
#define ERROR_GROUP_GAME_DEFINED
ERROR_GROUP(Game);
#endif
ERROR_SUBGROUP(Game, World);
ERROR_TYPE(Game_World, MapLoadFailed);

class CGame;
class CGameAttributes;
class CUnitManager;
class CEntityManager;
class CProjectileManager;
class CLOSManager;
class CTerritoryManager;
class CTerrain;

/**
 * CWorld is a general data class containing whatever is needed to accurately represent the world.
 * This includes the map, entities, influence maps, tiles, heightmap, etc.
 **/
class CWorld
{
	NONCOPYABLE(CWorld);
	/**
	 * pointer to the CGame object representing the game.
	 **/
	CGame *m_pGame;
	
	/**
	 * pointer to the CTerrain object representing the height map.
	 **/
	CTerrain *m_Terrain;

	/**
	 * pointer to the CUnitManager that holds all the units in the world. 
	 **/
	CUnitManager *m_UnitManager;
	/**
	 * pointer to the CEntityManager that holds all the entities in the world. 
	 **/
	CEntityManager *m_EntityManager;
	/**
	 * pointer to the CProjectileManager that holds all the projectiles in the world. 
	 **/
	CProjectileManager *m_ProjectileManager;
	/**
	 * pointer to the CLOSManager that holds the visibility matrix for the world. 
	 **/
	CLOSManager *m_LOSManager;
	/**
	 * pointer to the CTerritoryManager that holds territory matrix for the world. 
	 **/
	CTerritoryManager *m_TerritoryManager;

public:
	CWorld(CGame *pGame);
	~CWorld();

	void RegisterInit(CGameAttributes *pGameAttributes);
	/*
	Initialize the World - load the map and all objects
	*/
	void Initialize(CGameAttributes *pGameAttributes);

	// provided for JS _rewritemaps function
	void RewriteMap();

	/**
	 * Get the pointer to the terrain object.
	 *
	 * @return CTerrain * the value of m_Terrain.
	 **/
	inline CTerrain *GetTerrain()
	{	return m_Terrain; }

	/**
	 * Get a reference to the unit manager object.
	 *
	 * @return CUnitManager & dereferenced m_UnitManager.
	 **/
	inline CUnitManager &GetUnitManager()
	{	return *m_UnitManager; }
	/**
	 * Get a reference to the entity manager object.
	 *
	 * @return CWorld CEntityManager & dereferenced m_EntityManager.
	 **/
	inline CEntityManager &GetEntityManager()
	{	return *m_EntityManager; }
	/**
	 * Get a reference to the projectile manager object.
	 *
	 * @return CProjectileManager & dereferenced m_ProjectileManager.
	 **/
	inline CProjectileManager &GetProjectileManager()
	{	return *m_ProjectileManager; }
	/**
	 * Get the pointer to the LOS manager object.
	 *
	 * @return CLOSManager * the value of m_LOSManager.
	 **/
	inline CLOSManager *GetLOSManager()
	{	return m_LOSManager; }
	/**
	 * Get the pointer to the territory manager object.
	 *
	 * @return CTerritoryManager * the value of m_TerritoryManager.
	 **/
	inline CTerritoryManager *GetTerritoryManager()
	{	return m_TerritoryManager; }
};

// rationale: see definition.
class CLightEnv;
extern CLightEnv g_LightEnv;

#endif
