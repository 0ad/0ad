/**
 * =========================================================================
 * File        : DefaultEmitter.cpp
 * Project     : 0 A.D.
 * Description : Default particle emitter implementation.
 * =========================================================================
 */

#include "precompiled.h"
#include "maths/MathUtil.h"
#include "DefaultEmitter.h"

CDefaultEmitter::CDefaultEmitter(const int MAX_PARTICLES, const int lifetime) : CEmitter(MAX_PARTICLES, lifetime)
{
	Setup();
}

CDefaultEmitter::~CDefaultEmitter(void)
{
}

bool CDefaultEmitter::Setup()
{
	// XYZ Position
	pos.X = 0.0f;
	pos.Y = 20.0f;
	pos.Z = 0.0f;

	yaw = DEGTORAD(0.0f);
	yawVar = DEGTORAD(360.0f);
	pitch = DEGTORAD(90.0f);
	pitchVar = DEGTORAD(45.0f);
	speed = 0.05f;
	speedVar = 0.001f;

	blend_mode		= 1;
	particleCount	= 0;
	emitsPerFrame	= 100;
	emitVar	= 15;
	life = 90;
	lifeVar = 65;
	startColor.r = 100;
	startColor.g = 100;
	startColor.b = 100;
	startColorVar.r = 15;
	startColorVar.g = 15;
	startColorVar.b = 15;
	endColor.r = 0;
	endColor.g = 0;
	endColor.b = 0;
	endColorVar.r = 15;
	endColorVar.g = 15;
	endColorVar.b = 15;

	force.X = 0.000f;
	force.Y = -0.001f;
	force.Z = 0.0f;
	return true;
}

bool CDefaultEmitter::Update()
{
	int emits;
	// walk through the used list, and update each of the particles
	tParticle *tempParticle = usedList;			// start at the beginning of the used list
	tParticle *prev = usedList;					
	while(tempParticle)								// loop on a valid particle
	{
		// don't update if the particle is supposed to be dead
		if(tempParticle->life > 0)
		{
			// update the particle
			// Calculate the new pos
			tempParticle->pos.X += tempParticle->dir.X;
			tempParticle->pos.Y += tempParticle->dir.Y;
			tempParticle->pos.Z += tempParticle->dir.Z;

			// Add global force to direction
			tempParticle->dir.X += force.X;
			tempParticle->dir.Y += force.Y;
			tempParticle->dir.Z += force.Z;

			// Get the new color
			tempParticle->color.r += tempParticle->deltaColor.r;
			tempParticle->color.g += tempParticle->deltaColor.g;
			tempParticle->color.b += tempParticle->deltaColor.b;

			// fade it out
			if(decrementAlpha)
				tempParticle->alpha -= tempParticle->alphaDelta;

			// gets a little older
			if(decrementLife)
				tempParticle->life--;

			// move to the next particle in the list
			prev = tempParticle; 
			tempParticle = tempParticle->next;
		}
		else	// this means the particle lifetime is over
		{
			// if this is the first particle in usedList
			// then set the pointers to the next in the usedList
			// and open up the tempParticle
			if(tempParticle == usedList)
			{	
				usedList = tempParticle->next;
				tempParticle->next = openList;
				// set the open list head to the particle
				openList = tempParticle;
				prev = usedList;
				tempParticle = usedList;
			}
			else
			{

				//// We need to pull the particle out of the 
				//// used list and insert it into the open list
				
				// fix the previous node in the list to skip over the one we are pulling out
				prev->next = tempParticle->next;
				// set the particle to point to the head of the open list
				tempParticle->next = openList;
				// set the open list head to the particle
				openList = tempParticle;
				// move on to the next iteration
				tempParticle = prev->next;
			}
			// and there is one less
			particleCount--;
		}
	}	// end of while
	if(emitterLife > 0 || emitterLife == -1)
	{
		// Emit particles for this frame
		emits = emitsPerFrame + (int)((float)emitVar * RandomNum());

		// if the particle life is -1 that means it's infinite
		if(emitterLife != -1)
			emitterLife--;
		
		for(int i = 0; i < emits; i++)
			AddParticle();
		return true;
	}
	else
	{
		if(particleCount > 0)
		{	
			return true;
		}
		else
		{
			isFinished = true;
			return false;		// this will be checked for and then it will be deleted
		}
	}
}
