#ifndef _ps_Game_H
#define _ps_Game_H

// Kludge: Our included headers might want to subgroup the Game group, so do it
// here, before including the other guys
#include "ps/Errors.h"
ERROR_GROUP(Game);

#include "World.h"
#include "Simulation.h"
#include "GameView.h"

#include <vector>

class CPlayer;
class CGameAttributes;

// Default player limit (not counting the Gaia player)
// This may be overriden by system.cfg ("max_players")
#define PS_MAX_PLAYERS 6

class CGame
{
	CWorld m_World;
	CSimulation m_Simulation;
	CGameView m_GameView;
	
	CPlayer *m_pLocalPlayer;
	std::vector<CPlayer *> m_Players;
	uint m_NumPlayers;

	bool m_GameStarted;
	
public:
	CGame();
	~CGame();

	/*
		Initialize all local state and members for playing a game described by
		the attribute class, and start the game.

		Return: 0 on OK - a PSRETURN code otherwise
	*/
	PSRETURN StartGame(CGameAttributes *pGameAttributes);
	PSRETURN ReallyStartGame();

	/*
		Perform all per-frame updates
	*/
	void Update(double deltaTime);
	
	inline CPlayer *GetLocalPlayer()
	{	return m_pLocalPlayer; }
	inline void SetLocalPlayer(CPlayer *pLocalPlayer)
	{	m_pLocalPlayer=pLocalPlayer; }
	
	// PT: No longer inline, because it does too much error checking. When
	// everything stops trying to access players before they're loaded, feel
	// free to put the inline version back.
	CPlayer *GetPlayer(uint idx);

	inline std::vector<CPlayer*>* GetPlayers()
	{	return( &m_Players ); }

	inline uint GetNumPlayers()
	{	return m_NumPlayers; }

	inline bool IsGameStarted()
	{
		return m_GameStarted;
	}

	inline CWorld *GetWorld()
	{	return &m_World; }
	inline CGameView *GetView()
	{	return &m_GameView; }
	inline CSimulation *GetSimulation()
	{	return &m_Simulation; }

private:
	PSRETURN RegisterInit(CGameAttributes* pAttribs);

	// squelch "unable to generate" warnings
	CGame(const CGame& rhs);
	const CGame& operator=(const CGame& rhs);
};

extern CGame *g_Game;

#endif
