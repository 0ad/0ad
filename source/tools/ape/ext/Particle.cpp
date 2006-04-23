/*==================================================================
| 
| Name: Particle.cpp
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

#include "Particle.h"
#include "timer_.h"
#include <GL/gl.h>
#include <assert.h>

CParticle::CParticle() :
	m_duration(0.0f),
	m_timeElapsedTotal(0.0f),
	m_position(0.0f, 0.0f, 0.0f),
	m_velocity(0.0f, 0.0f, 0.0f),
	m_gravity(0.0f, 0.0f, 0.0f) 
{
	m_timeOfLastFrame = get_time();

}

CParticle::~CParticle() 
{
}

void CParticle::Init() 
{
	// calculate colour increment per second in order to fade to black
	m_colourInc[0] = - (m_colour[0] / m_duration);
	m_colourInc[1] = - (m_colour[1] / m_duration);
	m_colourInc[2] = - (m_colour[2] / m_duration);
}

void CParticle::Frame() 
{
	Update();
	Render();
}

void CParticle::Render() 
{
	assert(m_sprite);

	m_sprite->SetColour(m_colour);
	m_sprite->SetTranslation(m_position);
	m_sprite->Render();
}

void CParticle::Update() 
{
	float timeElapsed = float(get_time() - m_timeOfLastFrame);

	m_velocity += m_gravity * timeElapsed;
	m_position += m_velocity * timeElapsed;

	// fade colour
	m_colour[0] += m_colourInc[0] * timeElapsed;
	m_colour[1] += m_colourInc[1] * timeElapsed;
	m_colour[2] += m_colourInc[2] * timeElapsed;

	m_timeOfLastFrame = get_time();
	m_timeElapsedTotal += timeElapsed;
}

void CParticle::Reset() {
	m_duration = 0.0f;
	m_timeElapsedTotal = 0.0f;
	m_timeOfLastFrame = get_time();

	// default white colour
	m_colour[0] = m_colour[1] = m_colour[2] = m_colour[3] = 1.0f;	
}
