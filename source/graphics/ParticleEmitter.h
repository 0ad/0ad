/* Copyright (C) 2009 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Particle and Emitter base classes.
 */

#ifndef INCLUDED_PARTICLEEMITTER
#define INCLUDED_PARTICLEEMITTER

#include "lib/file/vfs/vfs_path.h"
#include "maths/Vector3D.h"
#include "ps/CStr.h"

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

	CStrW m_tag;
	int m_maxParticles;					// Maximum particles emitter can put out
	int	m_particleCount;				// Total emitted right now
	int	m_emitsPerFrame, m_emitsVar;	// Emits per frame and variation
	int m_emitterLife;					// Life of the emitter

	bool isFinished;					// tells the engine it's ready to be deleted

	// Transformation Info
	CVector3D m_pos;					// XYZ Position of emitter
	float m_yaw, m_yawVar;				// Yaw of emitted particles
	float m_pitch, m_pitchVar;			// Pitch of emitted particles
	float m_speed, m_speedVar;			// Speed of emitted particles
	
	// Particle linked lists
	tParticle* m_heap;					// Pointer to beginning of array
	tParticle* m_openList;				// linked list of unused particles
	tParticle* m_usedList;				// linked list of used particles	

	//Particle appearence
	CTexture* m_texture;				// Texture
	float m_size;						// size of the particles (if point sprites is not enabled)
	tColor m_startColor, m_startColorVar;	// Current color of particle
	tColor m_endColor, m_endColorVar;		// End color of particle
	int m_blendMode;					// Method used to blend particles
	int m_alpha, m_alphaVar;			// Alpha value for particles
	int	m_life, m_lifeVar;				// Life count and variation (in Frames)
	bool m_decrementLife;					// Controls whether or not the particles life is decremented every update.
	bool m_decrementAlpha;				// Controls whether or not the particles alpha is decremented every update.
	bool m_renderParticles;				// Controls the rendering of the particles.
	
	// Physics
	CVector3D m_force;						// Forces that affect the particles

public:
	CEmitter(const int MAX_PARTICLES = 4000, const int lifetime = -1, int textureID = 0);
	virtual ~CEmitter(void);

	// note: methods are virtual and overridable so as to suit the
	// specific particle needs.

	virtual bool LoadXml(const VfsPath& pathname);

	virtual bool Setup() { return false; }

	virtual bool AddParticle();

	virtual bool Update();

	virtual bool Render();

	// Helper functions
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
	CStrW GetTag() { return m_tag; }
	float GetPosX() { return m_pos.X; }
	float GetPosY() { return m_pos.Y; }
	float GetPosZ() { return m_pos.Z; }
	CVector3D GetPosVec() { return m_pos; }
	bool IsFinished(void) { return isFinished; }
	int GetEmitterLife() { return m_emitterLife; }
	int GetParticleCount() { return m_particleCount; }
	int GetMaxParticles(void) { return m_maxParticles; }
	tColor GetStartColor(void) { return m_startColor; }
	tColor GetStartColorVar(void) { return m_startColorVar; }
	tColor GetEndColor(void) { return m_endColor; }
	tColor GetEndColorVar(void) { return m_endColorVar; }
	int GetBlendMode(void) { return m_blendMode; }
	float GetSize(void) { return m_size; }
	float GetYaw(void) { return m_yaw; }
	float GetYawVar(void) { return m_yawVar; }
	float GetPitch(void) { return m_pitch; }
	float GetPitchVar(void) { return m_pitchVar; }
	float GetSpeed(void) { return m_speed; }
	float GetSpeedVar(void) { return m_speedVar; }
	int GetEmitsPerFrame(void) { return m_emitsPerFrame; }
	int GetEmitVar(void) { return m_emitsVar; }
	int GetLife(void) { return m_life; }
	int GetLifeVar(void) { return m_lifeVar; }
	float GetForceX(void) { return m_force.X; }
	float GetForceY(void) { return m_force.Y; }
	float GetForceZ(void) { return m_force.Z; }


	///////////////////////////////////////////////////////////////////
	//
	// Mutators	
	//
	///////////////////////////////////////////////////////////////////
	void SetTag(CStrW tag) { m_tag = tag; }
	void SetPosX(float posX) { m_pos.X = posX; }
	void SetPosY(float posY) { m_pos.Y = posY; }
	void SetPosZ(float posZ) { m_pos.Z = posZ; }
	inline void SetPosVec(const CVector3D& newPos)
	{
		m_pos = newPos;
	}
	void SetTexture(CTexture* id) { m_texture = id; }
	void SetIsFinished(bool finished) { isFinished = finished; }
	void SetEmitterLife(int life) { m_emitterLife = life; }
	void SetLife(int newlife) { m_life = newlife; }
	void SetLifeVar(int newlifevar) { m_lifeVar = newlifevar; }
	void SetSpeed(float newspeed) { m_speed = newspeed; }
	void SetSpeedVar(float newspeedvar) { m_speedVar = newspeedvar; }
	void SetYaw(float newyaw) { m_yaw = newyaw; }
	void SetYawVar(float newyawvar) { m_yawVar = newyawvar; }
	void SetPitch(float newpitch) { m_pitch = newpitch; }
	void SetPitchVar(float newpitchvar) { m_pitchVar = newpitchvar; }
	void SetStartColor(tColor newColor) { m_startColor = newColor; }
	void SetStartColorVar(tColor newColorVar) { m_startColorVar = newColorVar; }
	void SetEndColor(tColor newColor) { m_endColor = newColor; }
	void SetEndColorVar(tColor newColorVar) { m_endColorVar = newColorVar; }
	void SetStartColorR(int newColorR) { m_startColor.r = newColorR; }
	void SetStartColorG(int newColorG) { m_startColor.g = newColorG; }
	void SetStartColorB(int newColorB) { m_startColor.b = newColorB; }
	void SetStartColorVarR(int newColorVarR) { m_startColorVar.r = newColorVarR; }
	void SetStartColorVarG(int newColorVarG) { m_startColorVar.g = newColorVarG; }
	void SetStartColorVarB(int newColorVarB) { m_startColorVar.b = newColorVarB; }
	void SetEndColorR(int newColorR) { m_endColor.r = newColorR; }
	void SetEndColorG(int newColorG) { m_endColor.g = newColorG; }
	void SetEndColorB(int newColorB) { m_endColor.b = newColorB; }
	void SetEndColorVarR(int newColorVarR) { m_endColorVar.r = newColorVarR; }
	void SetEndColorVarG(int newColorVarG) { m_endColorVar.g = newColorVarG; }
	void SetEndColorVarB(int newColorVarB) { m_endColorVar.b = newColorVarB; }
	inline void SetBlendMode(int blendmode)
	{
		if(blendmode >= 1 && blendmode <= 4)
			m_blendMode = blendmode;
		else
			m_blendMode = 1;
	}
	void SetEmitsPerFrame(int emitsperframe) { m_emitsPerFrame = emitsperframe; }
	void SetEmitVar(int emitvar) { m_emitsVar = emitvar; }
	void SetForceX(float forceX) { m_force.X = forceX; }
	void SetForceY(float forceY) { m_force.Y = forceY; }
	void SetForceZ(float forceZ) { m_force.Z = forceZ; }
	void SetSize(float newSize) { m_size = newSize; }
	void SetRenderParticles(bool render) { m_renderParticles = render; }
};

#endif
