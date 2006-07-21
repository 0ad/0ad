#ifndef _ps_Game_H
#define _ps_Game_H

#include "ps/Errors.h"
#include "maths/MathUtil.h"
#include <vector>

class CWorld;
class CSimulation;
class CGameView;
class CSimulation;
class CPlayer;
class CGameAttributes;

// Default player limit (not counting the Gaia player)
// This may be overridden by system.cfg ("max_players")
#define PS_MAX_PLAYERS 8

class CGame
{
	CWorld *m_World;
	CSimulation *m_Simulation;
	CGameView *m_GameView;
	
	CPlayer *m_pLocalPlayer;
	std::vector<CPlayer *> m_Players;
	uint m_NumPlayers;

	bool m_GameStarted;

	float m_Time;
	float m_SimRate;

	enum EOG
	{
		EOG_NEUTRAL,
		EOG_DRAW,	//Draw by means of agreement of civilization
		EOG_SPECIAL_DRAW,	//Theoretically, players could die at the same time...?
		EOG_LOSE,
		EOG_WIN
	} GameStatus;
	
public:
	CGame();
	~CGame();

	bool m_Paused;

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
	void UpdateGameStatus();
	void EndGame();

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
	{	return m_World; }
	inline CGameView *GetView()
	{	return m_GameView; }
	inline CSimulation *GetSimulation()
	{	return m_Simulation; }
	
	inline float GetTime()
	{	return m_Time; }

	inline void SetSimRate(float simRate)
	{	 m_SimRate=clamp(simRate, 0.0f, simRate); }
	inline float GetSimRate()
	{	return m_SimRate;  }

private:
	PSRETURN RegisterInit(CGameAttributes* pAttribs);

	NO_COPY_CTOR(CGame);
};

extern CGame *g_Game;

#endif
