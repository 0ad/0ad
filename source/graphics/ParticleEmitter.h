/**
 * =========================================================================
 * File        : ParticleEmitter.h
 * Project     : 0 A.D.
 * Description : Particle and Emitter base classes.
 * =========================================================================
 */

#ifndef INCLUDED_PARTICLEEMITTER
#define INCLUDED_PARTICLEEMITTER

#include "maths/Vector3D.h"

class CTexture;

class CEmitter
{
	static const int HALF_RAND = (RAND_MAX / 2);

public:
	struct tColor
	{
		unsigned char r, g, b;
	};

	struct tParticle
	{
		// base stuff
		CVector3D pos;						// Current position					12
		CVector3D dir;						// Current direction with speed		12
		float	alpha;						// Fade value						4
		float	alphaDelta;					// Change of fade					4
		tColor	color;						// Current color of particle		3
		tColor  deltaColor;					// Change of color					3
		short	life;						// How long it will last			2

		// particle text stuff
		CVector3D endPos;					// For particle texture				12
		bool inPos;							//									1

		tParticle* next;					// pointer for link lists			4 

		tParticle()
		{
			next = 0;
		}
	};	

	//struct tParticleNode
	//{
	//	tParticle* pParticle;
	//	tParticleNode* next;
	//};	

protected:
	CTexture* texture;						// Texture ID
	bool isFinished;					// tells the engine it's ready to be deleted

	// Transformation Info
	CVector3D pos;						// XYZ Position
	CVector3D finalPos;					// Final position of the particles (IF IMPLOSION)
	float yaw, yawVar;					// Yaw and variation
	float pitch, pitchVar;				// Pitch and variation
	float speed, speedVar;				// Speed and variation
	float updateSpeed;					// Controls how fast emitter is updated.
	float size;							// size of the particles (if point sprites is not enabled)

	// Particle
	tParticle* heap;					// Pointer to beginning of array

	tParticle* openList;				// linked list of unused particles
	tParticle* usedList;				// linked list of used particles	

	int blend_mode;						// Method used to blend particles.
	int max_particles;					// Maximum particles emitter can put out
	int	particleCount;					// Total emitted right now
	int	emitsPerFrame, emitVar;			// Emits per frame and variation
	int	life, lifeVar;					// Life count and variation (in Frames)
	int emitterLife;					// Life of the emitter
	bool decrementLife;					// Controls whether or not the particles life is decremented every update.
	bool decrementAlpha;				// Controls whether or not the particles alpha is decremented every update.
	bool RenderParticles;				// Controls the rendering of the particles.
	tColor startColor, startColorVar;	// Current color of particle
	tColor endColor, endColorVar;		// Current color of particle

	// Physics
	CVector3D force;						// Forces that affect the particles

public:
	CEmitter(const int MAX_PARTICLES = 4000, const int lifetime = -1, int textureID = 0);
	virtual ~CEmitter(void);


	// note: functions are virtual and overridable so as to suit the
	// specific particle needs.

	virtual bool Setup() { return false; }

	virtual bool AddParticle();

	virtual bool Update() { return false; }

	virtual bool Render();

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

	inline void RotationToDirection(float pitch, float yaw, CVector3D* direction)
	{
		direction->X = (float)(-sin(yaw)*  cos(pitch));
		direction->Y = (float)sin(pitch);
		direction->Z = (float)(cos(pitch)*  cos(yaw));
	}

	///////////////////////////////////////////////////////////////////
	//
	// Accessors
	//
	///////////////////////////////////////////////////////////////////
	float GetPosX() { return pos.X; }
	float GetPosY() { return pos.Y; }
	float GetPosZ() { return pos.Z; }
	CVector3D GetPosVec() { return pos; }
	float GetFinalPosX() { return finalPos.X; }
	float GetFinalPosY() { return finalPos.Y; }
	float GetFinalPosZ() { return finalPos.Z; }
	bool IsFinished(void) { return isFinished; }
	int GetEmitterLife() { return emitterLife; }
	int GetParticleCount() { return particleCount; }
	float GetUpdateSpeed() { return updateSpeed; }
	int GetMaxParticles(void) { return max_particles; }
	tColor GetStartColor(void) { return startColor; }
	tColor GetStartColorVar(void) { return startColorVar; }
	tColor GetEndColor(void) { return endColor; }
	tColor GetEndColorVar(void) { return endColorVar; }
	int GetBlendMode(void) { return blend_mode; }
	float GetSize(void) { return size; }
	float GetYaw(void) { return yaw; }
	float GetYawVar(void) { return yawVar; }
	float GetPitch(void) { return pitch; }
	float GetPitchVar(void) { return pitchVar; }
	float GetSpeed(void) { return speed; }
	float GetSpeedVar(void) { return speedVar; }
	int GetEmitsPerFrame(void) { return emitsPerFrame; }
	int GetEmitVar(void) { return emitVar; }
	int GetLife(void) { return life; }
	int GetLifeVar(void) { return lifeVar; }
	float GetForceX(void) { return force.X; }
	float GetForceY(void) { return force.Y; }
	float GetForceZ(void) { return force.Z; }


	///////////////////////////////////////////////////////////////////
	//
	// Mutators	
	//
	///////////////////////////////////////////////////////////////////
	void SetPosX(float posX) { pos.X = posX; }
	void SetPosY(float posY) { pos.Y = posY; }
	void SetPosZ(float posZ) { pos.Z = posZ; }
	inline void SetPosVec(const CVector3D& newPos)
	{
		pos = newPos;
	}
	void SetFinalPosX(float finalposX) { finalPos.X = finalposX; }
	void SetFinalPosY(float finalposY) { finalPos.Y = finalposY; }
	void SetFinalPosZ(float finalposZ) { finalPos.Z = finalposZ; }
	void SetTexture(CTexture* id) { texture = id; }
	void SetIsFinished(bool finished) { isFinished = finished; }
	void SetEmitterLife(int life) { emitterLife = life; }
	void SetUpdateSpeed(float speed) { updateSpeed = speed; }
	void SetLife(int newlife) { life = newlife; }
	void SetLifeVar(int newlifevar) { lifeVar = newlifevar; }
	void SetSpeed(float newspeed) { speed = newspeed; }
	void SetSpeedVar(float newspeedvar) { speedVar = newspeedvar; }
	void SetYaw(float newyaw) { yaw = newyaw; }
	void SetYawVar(float newyawvar) { yawVar = newyawvar; }
	void SetPitch(float newpitch) { pitch = newpitch; }
	void SetPitchVar(float newpitchvar) { pitchVar = newpitchvar; }
	void SetStartColor(tColor newColor) { startColor = newColor; }
	void SetStartColorVar(tColor newColorVar) { startColorVar = newColorVar; }
	void SetEndColor(tColor newColor) { endColor = newColor; }
	void SetEndColorVar(tColor newColorVar) { endColorVar = newColorVar; }
	void SetStartColorR(int newColorR) { startColor.r = newColorR; }
	void SetStartColorG(int newColorG) { startColor.g = newColorG; }
	void SetStartColorB(int newColorB) { startColor.b = newColorB; }
	void SetStartColorVarR(int newColorVarR) { startColorVar.r = newColorVarR; }
	void SetStartColorVarG(int newColorVarG) { startColorVar.g = newColorVarG; }
	void SetStartColorVarB(int newColorVarB) { startColorVar.b = newColorVarB; }
	void SetEndColorR(int newColorR) { endColor.r = newColorR; }
	void SetEndColorG(int newColorG) { endColor.g = newColorG; }
	void SetEndColorB(int newColorB) { endColor.b = newColorB; }
	void SetEndColorVarR(int newColorVarR) { endColorVar.r = newColorVarR; }
	void SetEndColorVarG(int newColorVarG) { endColorVar.g = newColorVarG; }
	void SetEndColorVarB(int newColorVarB) { endColorVar.b = newColorVarB; }
	inline void SetBlendMode(int blendmode)
	{
		if(blendmode >= 1 && blendmode <= 4)
			blend_mode = blendmode;
		else
			blend_mode = 1;
	}
	void SetEmitsPerFrame(int emitsperframe) { emitsPerFrame = emitsperframe; }
	void SetEmitVar(int emitvar) { emitVar = emitvar; }
	void SetForceX(float forceX) { force.X = forceX; }
	void SetForceY(float forceY) { force.Y = forceY; }
	void SetForceZ(float forceZ) { force.Z = forceZ; }
	void SetSize(float newSize) { size = newSize; }
	void SetRenderParticles(bool render) { RenderParticles = render; }
};

#endif
