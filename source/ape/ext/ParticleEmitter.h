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
#include "Vector3D.h"
#include <vector>


//--------------------------------------------------------
//  Declarations
//--------------------------------------------------------

typedef struct {
	GLfloat x, y, z;
} vertex3f;

typedef struct {
	GLfloat x, y;
} vertex2f;

typedef struct {
	GLfloat r, g, b, a;
} color4f;

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

	bool IsFinished();

	void SetTimelimit(double time);

	// void SetSprite(CSprite * sprite); // deprecated
	void SetTexture(GLuint tex);
	void SetWidth(float width);
	void SetHeight(float height);

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
	GLuint m_texture;

	vertex3f *m_vertices;
	vertex2f *m_texCoords;
	color4f *m_colours;

	float m_width;
	float m_height;

	std::vector<CParticle *> m_particles;
	std::vector<CParticle *> m_disabled;

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
	double m_timeSinceLastEmit;
	double m_timeElapsedTotal;
	double m_timeLimit;
};

#endif // PARTICLE_EMITTER_H
