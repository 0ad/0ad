#ifndef _Simulation_H
#define _Simulation_H

class CGame;
class CGameAttributes;
class CWorld;

class CSimulation
{
	CGame *m_pGame;
	CWorld *m_pWorld;
	
	double m_DeltaTime;
	
	// Temp - Should move to a dynamic interval that can adapt to network
	// conditions
	const static int SIM_FRAMERATE = 10;
	const static int m_SimUpdateInterval=1000/SIM_FRAMERATE;
	
	// Simulate: move the deterministic simulation forward by one interval
	void Simulate();

	// Interpolate: interpolate a data point for rendering between simulation
	// frames.
	void Interpolate(double frameTime, double offset);
public:
	CSimulation(CGame *pGame);
	
	void Initialize(CGameAttributes *pGameAttributes);
	
	void Update(double frameTime);
};

#endif
