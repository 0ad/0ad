/*==================================================================
| 
| Name: ParticleEmitter.h
|
|===================================================================
|
| Author: Ben Vinegar
| Contact: benvinegar () hotmail ! com
|
|
| Last Modified: 03/08/04
|
| Overview: Particle emitter class that emits particles from
|           an origin (or area) with a variety of set colours,
|		    durations, forces and a single common sprite.
|
|
| Usage: Instantiate one emitter per desired effect. Set the
|        various fields (preferably all, the defaults are rather
|		 boring) and then call Frame() - you guessed it - every
|		 frame. 
|
| To do: TBA
|
| More Information: TBA
|
==================================================================*/

#ifndef PARTICLE_EMITTER_H
#define PARTICLE_EMITTER_H

//--------------------------------------------------------
//  Includes / Compiler directives
//--------------------------------------------------------

#include "Particle.h"
#include "Sprite.h"
#include "Vector3D.h"
#include <vector>


//--------------------------------------------------------
//  Declarations
//--------------------------------------------------------

class CParticleEmitter 
{
public:
	CParticleEmitter();
	~CParticleEmitter();

	// must be performed before first frame/render/update call
	bool Init();

	// renders and updates particles
	void Frame();

	// renders without updating particles
	void Render();
		
	void Update();

	void SetSprite(CSprite * sprite);

	void SetOrigin(CVector3D origin);
	void SetOrigin(float x, float y, float z);

	void SetOriginSpread(CVector3D spread);
	void SetOriginSpread(float x, float y, float z);
	
	void SetGravity(CVector3D gravity);
	void SetGravity(float x, float y, float z);

	void SetVelocity(CVector3D direction);
	void SetVelocity(float x, float y, float z);

	void SetVelocitySpread(CVector3D spread);
	void SetVelocitySpread(float x, float y, float z);

	void SetStartColour(float r, float g, float b, float a);
	void SetEndColour(float r, float g, float b, float a);

	// in milliseconds
	void SetMaxLifetime(double maxLife);

	// in milliseconds
	void SetMinLifetime(double minLife);

	void SetMaxParticles(int maxParticles);
	void SetMinParticles(int minParticles);

private:
	CSprite * m_sprite;

	std::vector<CParticle *> m_particles;
	
	CVector3D m_origin;
	CVector3D m_originSpread;

	CVector3D m_velocity;
	CVector3D m_velocitySpread;

	CVector3D m_gravity;

	float m_startColour[4];
	float m_endColour[4];

	int m_maxParticles;
	int m_minParticles;
	int m_numParticles;

	double m_maxLifetime;
	double m_minLifetime;
		
	double m_timeOfLastFrame;
	float  m_timeSinceLastEmit;
};

#endif // PARTICLE_EMITTER_H
