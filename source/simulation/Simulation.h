#ifndef _Simulation_H
#define _Simulation_H

#include <boost/random.hpp>

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

	// Random number generator
	boost::mt19937 m_Random;
	
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
	int Initialize(CGameAttributes *pGameAttributes);

	// Perform all CSimulation updates for the specified elapsed time.
	// (If frameTime=0, no simulation updates are done, but the graphics
	// are interpolated.)
	void Update(double frameTime);

	// Calculate the message mask of a message to be queued
	static uint GetMessageMask(CNetMessage *, uint oldMask, void *userdata);
	
	// Translate the command message into an entity order and queue it
	// Returns oldMask
	static uint TranslateMessage(CNetMessage *, uint oldMask, void *userdata);

	void QueueLocalCommand(CNetMessage *pMsg);

	// Get a random integer between 0 and maxVal-1 from the simulation's random number generator
	int RandInt(int maxVal);

	// Get a random float in [0, 1) from the simulation's random number generator
	float RandFloat();
};

#endif
