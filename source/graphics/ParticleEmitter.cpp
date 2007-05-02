/////////////////////////////////////////////////////
//	File Name:	ParticleEmitter.cpp
//	Date:		6/29/05
//	Author:		Will Dull
//	Purpose:	The base particle and emitter
//				classes implementations.
/////////////////////////////////////////////////////

#include "precompiled.h"
#include "ParticleEmitter.h"
#include "ParticleEngine.h"

CEmitter::CEmitter(const int MAX_PARTICLES, const int lifetime, int UNUSED(textureID))
{
	particleCount = 0;
	// declare the pool of nodes
	max_particles = MAX_PARTICLES;
	heap = new tParticle[max_particles];
	emitterLife = lifetime;
	decrementLife = true;
	decrementAlpha = true;
	RenderParticles = true;
	isFinished = false;
	updateSpeed = 0.02f;
	blend_mode		= 1;
	size = 0.15f;
	//texture = textureID;
	texture = 0;
	
	// init the used/open list
	usedList = NULL;
	openList = NULL;

	// link all the particles in the heap
	//	into one large open list
	for(int i = 0; i < max_particles - 1; i++)
	{
		heap[i].next = &(heap[i + 1]);	 
	}
	openList = heap;
}

CEmitter::~CEmitter(void)
{
	delete [] heap;
}

bool CEmitter::AddParticle()
{
	tColor start, end;
	float fYaw, fPitch, fSpeed;

	if(!openList)
		return false;

	if(particleCount < max_particles)
	{
		// get a particle from the open list
		tParticle *particle = openList;

		// set it's initial position to the emitter's position
		particle->pos.X = pos.X;
		particle->pos.Y = pos.Y;
		particle->pos.Z = pos.Z;

		// Calculate the starting direction vector
		fYaw = yaw + (yawVar * RandomNum());
		fPitch = pitch + (pitchVar * RandomNum());

		// Convert the rotations to a vector
		RotationToDirection(fPitch,fYaw,&particle->dir);

		// Multiply in the speed factor
		fSpeed = speed + (speedVar * RandomNum());
		particle->dir.X *= fSpeed;
		particle->dir.Y *= fSpeed;
		particle->dir.Z *= fSpeed;

		// Calculate the life span
		particle->life = life + (int)((float)lifeVar * RandomNum());

		// Calculate the colors
		start.r = startColor.r + (startColorVar.r * RandomChar());
		start.g = startColor.g + (startColorVar.g * RandomChar());
		start.b = startColor.b + (startColorVar.b * RandomChar());
		end.r = endColor.r + (endColorVar.r * RandomChar());
		end.g = endColor.g + (endColorVar.g * RandomChar());
		end.b = endColor.b + (endColorVar.b * RandomChar());

		// set the initial color of the particle
		particle->color.r = start.r;
		particle->color.g = start.g;
		particle->color.b = start.b;

		// Create the color delta
		particle->deltaColor.r = (end.r - start.r) / particle->life;
		particle->deltaColor.g = (end.g - start.g) / particle->life;
		particle->deltaColor.b = (end.b - start.b) / particle->life;

		particle->alpha = 255.0f;
		particle->alphaDelta = particle->alpha / particle->life;

		particle->inPos = false;

		// Now, we pop a node from the open list and put it into the used list
		openList = particle->next;
		particle->next = usedList;
		usedList = particle;

		// update the length of the used list (particle Count)
		particleCount++;
		return true;
	}

	return false;
}

bool CEmitter::Render()
{
	if(RenderParticles)
	{
		switch(blend_mode)
		{
		case 1:
			glBlendFunc(GL_SRC_ALPHA, GL_ONE);						// Fire
			break;
		case 2:
			glBlendFunc(GL_SRC_COLOR, GL_ONE);						// Crappy Fire
			break;
		case 3:
			glBlendFunc(GL_SRC_COLOR, GL_ONE_MINUS_SRC_COLOR);		// Plain Particles
			break;
		case 4:
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_COLOR);		// Nice fade out effect
			break;
		}

		// Bind the texture. Use the texture assigned to this emitter.
		int unit = 0;
		g_Renderer.SetTexture(unit, texture);

		glBegin(GL_QUADS);
		{
			tParticle *tempParticle = usedList;
		
			while(tempParticle)
			{
				tColor *pColor = &(tempParticle->color);
				glColor4ub(pColor->r,pColor->g, pColor->b, (GLubyte)tempParticle->alpha);
				glTexCoord2d(0.0, 0.0);
				CVector3D *pPos = &(tempParticle->pos);
				glVertex3f(pPos->X - size, pPos->Y + size, pPos->Z);
				glTexCoord2d(0.0, 1.0);
				glVertex3f(pPos->X - size, pPos->Y - size, pPos->Z);
				glTexCoord2d(1.0, 1.0);
				glVertex3f(pPos->X + size, pPos->Y - size, pPos->Z);
				glTexCoord2d(1.0, 0.0);
				glVertex3f(pPos->X + size, pPos->Y + size, pPos->Z);
				tempParticle = tempParticle->next;
			}
		}
		glEnd();

		return true;
	}
	return false;
}
