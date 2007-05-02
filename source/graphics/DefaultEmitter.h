/////////////////////////////////////////////////////
//	File Name:	DefaultEmitter.h
//	Date:		7/20/05
//	Author:		Will Dull
//	Purpose:	Default emitter
/////////////////////////////////////////////////////

#ifndef _DEFAULTEMITTER_H_
#define _DEFAULTEMITTER_H_

#include "ParticleEmitter.h"

class CDefaultEmitter : public CEmitter
{
public:
	CDefaultEmitter(const int MAX_PARTICLES = 4000, const int lifetime = -1);
	virtual ~CDefaultEmitter(void);

	// Sets up emitter to the default particle effect.
	virtual bool Setup();

	virtual bool Update();
};

#endif
