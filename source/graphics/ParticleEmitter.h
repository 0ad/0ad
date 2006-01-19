/////////////////////////////////////////////////////
//	File Name:	ParticleEmitter.h
//	Date:		6/29/05
//	Author:		Will Dull
//	Purpose:	The base particle and emitter
//				classes.
/////////////////////////////////////////////////////

#ifndef _PARTICLEEMITTER_H_
#define _PARTICLEEMITTER_H_

#include "precompiled.h"
#include <iostream>
#include <windows.h>
#include "ogl.h"
#include <math.h>

#define M_PI        3.14159265358979323846f
#define HALF_PI	    1.57079632679489661923f

#define DEGTORAD(d)	((d * (float)M_PI) / 180.0f);
#define RADTODEG(r)	((r * 180.0f) /(float)M_PI);

const int HALF_RAND = (RAND_MAX / 2);

struct tVector
{
	float x,y,z;
};

class CEmitter
{
public:
	struct tColor
	{
		unsigned char r, g, b;
	};

	struct tParticle
	{
		// base stuff
		tVector pos;						// Current position					12
		tVector dir;						// Current direction with speed		12
		float	alpha;						// Fade value						4
		float	alphaDelta;					// Change of fade					4
		tColor	color;						// Current color of particle		3
		tColor  deltaColor;					// Change of color					3
		short	life;						// How long it will last			2

		// particle text stuff
		tVector endPos;						// For particle texture				12
		bool inPos;							//									1

		tParticle *next;					// pointer for link lists			4 

		tParticle()
		{
			next = 0;
		}
	};	

	//struct tParticleNode
	//{
	//	tParticle *pParticle;
	//	tParticleNode *next;
	//};	

protected:
	int texture;						// Texture ID
	bool isFinished;					// tells the engine it's ready to be deleted

	// Transformation Info
	tVector	pos;						// XYZ Position
	tVector finalPos;					// Final position of the particles (IF IMPLOSION)
	float yaw, yawVar;					// Yaw and variation
	float pitch, pitchVar;				// Pitch and variation
	float speed, speedVar;				// Speed and variation
	float updateSpeed;					// Controls how fast emitter is updated.
	float size;							// size of the particles (if point sprites is not enabled)

	// Particle
	tParticle *heap;					// Pointer to beginning of array

	tParticle *openList;				// linked list of unused particles
	tParticle *usedList;				// linked list of used particles	

	int blend_mode;						// Method used to blend particles.
	int max_particles;					// Maximum particles emitter can put out
	int	particleCount;					// Total emitted right now
	int	emitsPerFrame, emitVar;			// Emits per frame and variation
	int	life, lifeVar;					// Life count and variation (in Frames)
	int emitterLife;					// Life of the emitter
	bool decrementLife;					// Controls whether or not the particles life is decremented every update.
	bool decrementAlpha;				// Controls whether or not the particles alpha is decremented every update.
	bool renderParticles;				// Controls the rendering of the particles.
	tColor startColor, startColorVar;	// Current color of particle
	tColor endColor, endColorVar;		// Current color of particle

	// Physics
	tVector force;						// Forces that affect the particles

public:
	// Constructor
	CEmitter(const int MAX_PARTICLES = 4000, const int lifetime = -1, int textureID = 0);

	//////////////////////////////////////////////////////////////
	//Func Name: setupEmitter
	//Date: 9/18/05
	//Author: Will Dull
	//Notes: Setup emitter. Setup so that a derived class can
	//		overload this function to suit the specific particles 
	//		needs.
	//////////////////////////////////////////////////////////////
	virtual bool setupEmitter() { return false;}

	//////////////////////////////////////////////////////////////
	//Func Name: addParticle
	//Date: 9/18/05
	//Author: Will Dull
	//Notes: Sets up and adds a particle to an emitter. Setup so
	//		that a derived class can overload this function to
	//		suit the specific particles needs.
	//////////////////////////////////////////////////////////////
	virtual bool addParticle();

	//////////////////////////////////////////////////////////////
	//Func Name: updateEmitter
	//Date: 9/18/05
	//Author: Will Dull
	//Notes: Updates emitter. Setup so that a derived class can 
	//		overload this function to suit the specific particles 
	//		needs.
	//////////////////////////////////////////////////////////////
	virtual bool updateEmitter() { return false; }

	//////////////////////////////////////////////////////////////
	//Func Name: renderEmitter
	//Date: 9/18/05
	//Author: Will Dull
	//Notes: Renders emitter. Setup so that a derived class can 
	//		overload this function to suit the specific particles 
	//		needs.
	//////////////////////////////////////////////////////////////
	virtual bool renderEmitter();

	inline float RandomNum()
	{
		int rn;
		rn = rand();
		return ((float)(rn - HALF_RAND) / (float)HALF_RAND);
	}

	inline char RandomChar()
	{
		return (unsigned char)(rand() >> 24);
	}

	inline void RotationToDirection(float pitch, float yaw, tVector *direction)
	{
		direction->x = (float)(-sin(yaw) * cos(pitch));
		direction->y = (float)sin(pitch);
		direction->z = (float)(cos(pitch) * cos(yaw));
	}

	///////////////////////////////////////////////////////////////////
	//
	// Accessors
	//
	///////////////////////////////////////////////////////////////////
	float getPosX() { return pos.x; }
	float getPosY() { return pos.y; }
	float getPosZ() { return pos.z; }
	tVector getPosVec() { return pos; }
	float getFinalPosX() { return finalPos.x; }
	float getFinalPosY() { return finalPos.y; }
	float getFinalPosZ() { return finalPos.z; }
	bool getIsFinished(void) { return isFinished; }
	int getEmitterLife() { return emitterLife; }
	int getParticleCount() { return particleCount; }
	float getUpdateSpeed() { return updateSpeed; }
	int getMaxParticles(void) { return max_particles; }
	tColor getStartColor(void) { return startColor; }
	tColor getStartColorVar(void) { return startColorVar; }
	tColor getEndColor(void) { return endColor; }
	tColor getEndColorVar(void) { return endColorVar; }
	int getBlendMode(void) { return blend_mode; }
	float getSize(void) { return size; }
	float getYaw(void) { return yaw; }
	float getYawVar(void) { return yawVar; }
	float getPitch(void) { return pitch; }
	float getPitchVar(void) { return pitchVar; }
	float getSpeed(void) { return speed; }
	float getSpeedVar(void) { return speedVar; }
	int getEmitsPerFrame(void) { return emitsPerFrame; }
	int getEmitVar(void) { return emitVar; }
	int getLife(void) { return life; }
	int getLifeVar(void) { return lifeVar; }
	float getForceX(void) { return force.x; }
	float getForceY(void) { return force.y; }
	float getForceZ(void) { return force.z; }


	///////////////////////////////////////////////////////////////////
	//
	// Mutators	
	//
	///////////////////////////////////////////////////////////////////
	void setPosX(float posX) { pos.x = posX; }
	void setPosY(float posY) { pos.y = posY; }
	void setPosZ(float posZ) { pos.z = posZ; }
	inline void setPosVec(tVector newPos)
	{
		pos.x = newPos.x;
		pos.y = newPos.y;
		pos.z = newPos.z;
	}
	void setFinalPosX(float finalposX) { finalPos.x = finalposX; }
	void setFinalPosY(float finalposY) { finalPos.y = finalposY; }
	void setFinalPosZ(float finalposZ) { finalPos.z = finalposZ; }
	void setTexture(int id) { texture = id; }
	void setIsFinished(bool finished) { isFinished = finished; }
	void setEmitterLife(int life) { emitterLife = life; }
	void setUpdateSpeed(float speed) { updateSpeed = speed; }
	void setLife(int newlife) { life = newlife; }
	void setLifeVar(int newlifevar) { lifeVar = newlifevar; }
	void setSpeed(float newspeed) { speed = newspeed; }
	void setSpeedVar(float newspeedvar) { speedVar = newspeedvar; }
	void setYaw(float newyaw) { yaw = newyaw; }
	void setYawVar(float newyawvar) { yawVar = newyawvar; }
	void setPitch(float newpitch) { pitch = newpitch; }
	void setPitchVar(float newpitchvar) { pitchVar = newpitchvar; }
	void setStartColor(tColor newColor) { startColor = newColor; }
	void setStartColorVar(tColor newColorVar) { startColorVar = newColorVar; }
	void setEndColor(tColor newColor) { endColor = newColor; }
	void setEndColorVar(tColor newColorVar) { endColorVar = newColorVar; }
	void setStartColorR(int newColorR) { startColor.r = newColorR; }
	void setStartColorG(int newColorG) { startColor.g = newColorG; }
	void setStartColorB(int newColorB) { startColor.b = newColorB; }
	void setStartColorVarR(int newColorVarR) { startColorVar.r = newColorVarR; }
	void setStartColorVarG(int newColorVarG) { startColorVar.g = newColorVarG; }
	void setStartColorVarB(int newColorVarB) { startColorVar.b = newColorVarB; }
	void setEndColorR(int newColorR) { endColor.r = newColorR; }
	void setEndColorG(int newColorG) { endColor.g = newColorG; }
	void setEndColorB(int newColorB) { endColor.b = newColorB; }
	void setEndColorVarR(int newColorVarR) { endColorVar.r = newColorVarR; }
	void setEndColorVarG(int newColorVarG) { endColorVar.g = newColorVarG; }
	void setEndColorVarB(int newColorVarB) { endColorVar.b = newColorVarB; }
	inline void setBlendMode(int blendmode)
	{
		if(blendmode >= 1 && blendmode <= 4)
			blend_mode = blendmode;
		else
			blend_mode = 1;
	}
	void setEmitsPerFrame(int emitsperframe) { emitsPerFrame = emitsperframe; }
	void setEmitVar(int emitvar) { emitVar = emitvar; }
	void setForceX(float forceX) { force.x = forceX; }
	void setForceY(float forceY) { force.y = forceY; }
	void setForceZ(float forceZ) { force.z = forceZ; }
	void setSize(float newSize) { size = newSize; }
	void setRenderParticles(bool render) { renderParticles = render; }


	// Destructor
	virtual ~CEmitter(void);
};

#endif