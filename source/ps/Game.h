#ifndef _ps_Game_H
#define _ps_Game_H

// Kludge: Our included headers might want to subgroup the Game group, so do it
// here, before including the other guys
#include "Errors.h"
ERROR_GROUP(Game);

#include "World.h"
#include "Simulation.h"
#include "Player.h"
#include "GameView.h"
#include "scripting/ScriptingHost.h"

#include <vector>

class CGameAttributes
{
public:
	inline CGameAttributes():
		m_MapFile()
	{}

	JSBool FillFromJS(JSContext *cx, JSObject *obj);

	// The VFS path of the mapfile to load or NULL for no map (and to use
	// default terrain)
	CStr m_MapFile;
};

class CGame
{
	CWorld m_World;
	CSimulation m_Simulation;
	CGameView m_GameView;
	
	std::vector<CPlayer *> m_Players;
	CPlayer *m_pLocalPlayer;
	
public:
	CGame();
	~CGame();

	/*
		Initialize all local state and members for playing a game described by
		the attribute class.
		
		Return: 0 on OK - a PSRETURN code otherwise
	*/
	PSRETURN Initialize(CGameAttributes *pGameAttributes);
	
	/*
		Perform all per-frame updates
	*/
	void Update(double deltaTime);
	
	inline CWorld *GetWorld()
	{	return &m_World; }
	inline CGameView *GetView()
	{	return &m_GameView; }
};

extern CGame *g_Game;

#endif
