/*==================================================================
| 
| Name: Particle.h
|
|===================================================================
|
| Author: Ben Vinegar
| Contact: benvinegar () hotmail ! com
|
|
| Last Modified: 03/08/04
|
| Overview: A single particle, currently only utilized by
|           CParticleEmitter. Public variables are for performance
|           reasons.
|
|
| Usage: Instantiate a particle, set public variables, then call
|        Frame() every frame.
|
| To do: TBA
|
| More Information: TBA
|
==================================================================*/

#ifndef PARTICLE_H
#define PARTICLE_H

//--------------------------------------------------------
//  Includes / Compiler directives
//--------------------------------------------------------

#include "Vector3D.h"
#include "Sprite.h"

//--------------------------------------------------------
//  Declarations
//--------------------------------------------------------

class CParticle 
{
public:
	CParticle();
	~CParticle();

	// necessary pre-processing immediately before first update call
	void Init();

	void Frame();
	void Update();
	void Render();

	void SetColour(float r, float g, float b, float a);

	CSprite * m_sprite;

	float m_duration;
	double m_timeOfLastFrame;
	double m_timeElapsedTotal;

	CVector3D m_position;
	CVector3D m_velocity;
	CVector3D m_gravity;

	float m_colour[4];
	float m_colourInc[3];
};


#endif // PARTICLE_H