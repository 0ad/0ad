/*==================================================================
| 
| Name: ParticleSystem.h
|
|===================================================================
|
| Author: Ben Vinegar
| Contact: benvinegar () hotmail ! com
|
|
| Last Modified: 04/23/04
|
| Overview: TBA
|
| Usage: TBA
|
| To do: TBA
|
| More Information: TBA
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