#ifndef _Simulation_H
#define _Simulation_H

#include "lib/types.h"

class CGame;
class CGameAttributes;
class CWorld;
class CTurnManager;
class CNetMessage;

class CSimulation
{
	CGame *m_pGame;
	CWorld *m_pWorld;
	CTurnManager *m_pTurnManager;
	
	double m_DeltaTime;
	
	// Simulate: move the deterministic simulation forward by one interval
	void Simulate();

	// Interpolate: interpolate a data point for rendering between simulation
	// frames.
	void Interpolate(double frameTime, double offset);

public:
	CSimulation(CGame *pGame);
	~CSimulation();
	
	inline void SetTurnManager(CTurnManager *pTurnManager)
	{	m_pTurnManager=pTurnManager; }
	inline CTurnManager *GetTurnManager()
	{	return m_pTurnManager; }

	void RegisterInit(CGameAttributes *pGameAttributes);
	void Initialize(CGameAttributes *pGameAttributes);

	// Perform all CSimulation updates for the specified elapsed time.
	void Update(double frameTime);

	static uint GetMessageMask(CNetMessage *, uint, void *);
	// Translate the command message into an entity order and queue it
	static uint TranslateMessage(CNetMessage *, uint, void *);

	void QueueLocalCommand(CNetMessage *pMsg);
};

#endif
