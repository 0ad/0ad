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

#include "AttributeMap.h"

#include <vector>

class CGameAttributes: public CAttributeMap
{
protected:
	virtual void CreateJSObject();
	virtual JSBool GetJSProperty(jsval id, jsval *ret);

public:
	CGameAttributes();
	virtual ~CGameAttributes();

	// NOTE: Public only for JS interface
	class CPlayerAttributes: public CAttributeMap
	{
	protected:
		virtual void CreateJSObject();
	public:
		CPlayerAttributes();
	};

	JSObject *m_PlayerArrayJS;
	std::vector <CPlayerAttributes *> m_PlayerAttribs;
};

class CGame
{
	CWorld m_World;
	CSimulation m_Simulation;
	CGameView m_GameView;
	
	std::vector<CPlayer *> m_Players;
	CPlayer *m_pLocalPlayer;
	uint m_NumPlayers;
	
public:
	CGame();
	~CGame();

	/*
		Initialize all local state and members for playing a game described by
		the attribute class, and start the game.

		Return: 0 on OK - a PSRETURN code otherwise
	*/
	PSRETURN StartGame(CGameAttributes *pGameAttributes);
	
	/*
		Perform all per-frame updates
	*/
	void Update(double deltaTime);
	
	inline CPlayer *GetLocalPlayer()
	{	return m_pLocalPlayer; }

	inline uint GetNumPlayers()
	{	return m_NumPlayers; }

	inline CWorld *GetWorld()
	{	return &m_World; }
	inline CGameView *GetView()
	{	return &m_GameView; }
	inline CSimulation *GetSimulation()
	{	return &m_Simulation; }
};

extern CGame *g_Game;

#endif
