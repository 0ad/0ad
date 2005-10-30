/*==================================================================
| 
| Name: ParticleEmitter.cpp
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

#include "precompiled.h"

#include "ParticleEmitter.h"
#include "timer.h"
#include "ogl.h"
#include <stdlib.h>

using namespace std;

CParticleEmitter::CParticleEmitter() :
	m_origin(0.0f, 0.0f, 0.0f),
	m_originSpread(0.0f, 0.0f, 0.0f),
	m_velocity(0.0f, 0.0f, 0.0f),
	m_velocitySpread(0.0f, 0.0f, 0.0f),
	m_gravity(0.0f, 0.0f, 0.0f),
	m_maxParticles(0),
	m_minParticles(0),
	m_numParticles(0),
	m_maxLifetime(0),
	m_minLifetime(0),
	m_timeOfLastFrame(0.0f),
	m_timeSinceLastEmit(0.0f) 
{
	m_particles.clear();
}

CParticleEmitter::~CParticleEmitter() 
{
}

void CParticleEmitter::Frame() 
{
	Update();
	Render();
}

void CParticleEmitter::Render() 
{
	
	glEnable(GL_ALPHA_TEST);
	glEnable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);

	glAlphaFunc(GL_GREATER, 0.0f);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE);

	vector<CParticle *>::iterator itor = m_particles.begin();
	while (itor != m_particles.end()) 
	{
		CParticle * curParticle = (*itor);
		
		curParticle->Frame();
		++itor;
	}
	glDisable(GL_ALPHA_TEST);
	glDisable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
}

void CParticleEmitter::Update() 
{
	double timeElapsed = get_time() - m_timeOfLastFrame;

	// update existing particles
	vector<CParticle *>::iterator itor = m_particles.begin();
	while (itor != m_particles.end()) 
	{
		CParticle * curParticle = (*itor);

		curParticle->Update();

		// destroy particle if it has lived beyond its duration
		if (curParticle->m_timeElapsedTotal >= curParticle->m_duration) 
		{
			m_particles.erase(itor);
			delete curParticle;
			--m_numParticles;
		}

		++itor;
	}

	double secondsPerEmit = 1 / (m_minParticles / m_minLifetime);

	if (m_timeSinceLastEmit > secondsPerEmit) 
	{
		
		float duration;
		CVector3D position, velocity;
		float colour[4];

		bool moreParticlesToEmit = true;
		while (moreParticlesToEmit) {
			CParticle * newParticle = new CParticle();
			
			// calculate particle duration
			duration = (float)m_minLifetime;
			duration += (rand() % (int)((m_maxLifetime - m_minLifetime) * 1000.0f + 1)) / 1000.0f;
			newParticle->m_duration = duration;

			// calculate particle start position from spread
			position = m_origin;
			position.X += (rand() % (int)(m_originSpread.X * 2000.0f + 1)) / 1000.0f - m_originSpread.X;
			position.Y += (rand() % (int)(m_originSpread.Y * 2000.0f + 1)) / 1000.0f - m_originSpread.Y;
			position.Z += (rand() % (int)(m_originSpread.Z * 2000.0f + 1)) / 1000.0f - m_originSpread.Z;
			newParticle->m_position = position;

			// calculate particle velocity from spread
			velocity = m_velocity;
			velocity.X += (rand() % (int)(m_velocitySpread.X * 2000.0f + 1)) / 1000.0f - m_velocitySpread.X;
			velocity.Y += (rand() % (int)(m_velocitySpread.Y * 2000.0f + 1)) / 1000.0f - m_velocitySpread.Y;
			velocity.Z += (rand() % (int)(m_velocitySpread.Z * 2000.0f + 1)) / 1000.0f - m_velocitySpread.Z;
			newParticle->m_velocity = velocity;

			newParticle->m_gravity = m_gravity;

			// calculate and assign colour
			memcpy2(colour, m_startColour, sizeof(float) * 4);
			colour[0] += (rand() % (int)((m_endColour[0] - m_startColour[0]) * 1000.0f + 1)) / 1000.0f;
			colour[1] += (rand() % (int)((m_endColour[1] - m_startColour[1]) * 1000.0f + 1)) / 1000.0f;
			colour[2] += (rand() % (int)((m_endColour[2] - m_startColour[2]) * 1000.0f + 1)) / 1000.0f;
			colour[3] += (rand() % (int)((m_endColour[3] - m_startColour[3]) * 1000.0f + 1)) / 1000.0f;
			memcpy2(newParticle->m_colour, colour, sizeof(float) * 4);

			// assign sprite
			newParticle->m_sprite = m_sprite;

			// final pre-processing init call
			newParticle->Init();

			// add to vector of particles
			m_particles.push_back(newParticle);

			timeElapsed -= secondsPerEmit;
			if (timeElapsed < secondsPerEmit) 
			{
				moreParticlesToEmit = false;
			}


			++m_numParticles;
		}
		m_timeSinceLastEmit = 0.0f;
	}
	else
		m_timeSinceLastEmit += (float)timeElapsed;

	m_timeOfLastFrame = get_time();
}

void CParticleEmitter::SetSprite(CSprite * sprite) 
{
	m_sprite = sprite;
}

void CParticleEmitter::SetOrigin(CVector3D origin) 
{
	m_origin = origin;
}

void CParticleEmitter::SetOrigin(float x, float y, float z) 
{
	m_origin.X = x;
	m_origin.Y = y;
	m_origin.Z = z;
}

void CParticleEmitter::SetOriginSpread(CVector3D spread) 
{
	m_originSpread = spread;
}

void CParticleEmitter::SetOriginSpread(float x, float y, float z) 
{
	m_originSpread.X = x;
	m_originSpread.Y = y;
	m_originSpread.Z = z;
}

void CParticleEmitter::SetGravity(CVector3D gravity) 
{
	m_gravity = gravity;
}

void CParticleEmitter::SetGravity(float x, float y, float z) 
{
	m_gravity.X = x;
	m_gravity.Y = y;
	m_gravity.Z = z;

}

void CParticleEmitter::SetVelocity(CVector3D velocity) 
{
	m_velocity = velocity;
}

void CParticleEmitter::SetVelocity(float x, float y, float z) 
{
	m_velocity.X = x;
	m_velocity.Y = y;
	m_velocity.Z = z;
}


void CParticleEmitter::SetVelocitySpread(CVector3D spread) 
{
	m_velocitySpread = spread;
}

void CParticleEmitter::SetVelocitySpread(float x, float y, float z) 
{
	m_velocitySpread.X = x;
	m_velocitySpread.Y = y;
	m_velocitySpread.Z = z;
}

void CParticleEmitter::SetStartColour(float r, float g, float b, float a) 
{
	m_startColour[0] = r;
	m_startColour[1] = g;
	m_startColour[2] = b;
	m_startColour[3] = a;
}

void CParticleEmitter::SetEndColour(float r, float g, float b, float a) 
{
	m_endColour[0] = r;
	m_endColour[1] = g;
	m_endColour[2] = b;
	m_endColour[3] = a;
}

void CParticleEmitter::SetMaxLifetime(double maxLife) 
{
	m_maxLifetime = maxLife;
}

void CParticleEmitter::SetMinLifetime(double minLife) 
{
	m_minLifetime = minLife;
}

void CParticleEmitter::SetMaxParticles(int maxParticles) 
{
	m_maxParticles = maxParticles;
}

void CParticleEmitter::SetMinParticles(int minParticles) 
{
	m_minParticles = minParticles;
}
