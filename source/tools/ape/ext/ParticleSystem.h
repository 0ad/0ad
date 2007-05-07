/*==================================================================
| 
| Name: ParticleSystem.h
|
|===================================================================
|
==================================================================*/

#ifndef PARTICLE_SYSTEM_H
#define PARTICLE_SYSTEM_H

#include "ParticleEmitter.h"
#include <vector>

class CParticleSystem
{
public:
	CParticleSystem();
	~CParticleSystem();

	void Render();
	void Update();
	void Frame();

	CParticleEmitter *CreateNewEmitter();
private:
	std::vector<CParticleEmitter *> m_Emitters;
};


#endif // PARTICLE_SYSTEM_H
