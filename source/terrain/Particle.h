//***********************************************************
//
// Name:		Particle.h
// Last Update: 03/04/04
// Author:		Ben Vinegar
//
// Description: Particle class header
//
//***********************************************************

#ifndef PARTICLE_H
#define PARTICLE_H

#include "Vector3D.h"
#include "Sprite.h"

class CParticle {
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