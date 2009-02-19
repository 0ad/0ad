/**
 * =========================================================================
 * File        : DefaultEmitter.h
 * Project     : 0 A.D.
 * Description : Default particle emitter implementation.
 * =========================================================================
 */

#ifndef INCLUDED_DEFAULTEMITTER
#define INCLUDED_DEFAULTEMITTER

#include "ParticleEmitter.h"

class CDefaultEmitter : public CEmitter
{
public:
	CDefaultEmitter(const int MAX_PARTICLES = 4000, const int lifetime = -1);
	virtual ~CDefaultEmitter(void);

	// Sets up emitter to the default particle effect.
	virtual bool Setup();

};

#endif
