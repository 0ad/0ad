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

	//////////////////////////////////////////////////////////////
	//Func Name: setupEmitter
	//Date: 7/19/05
	//Author: Will Dull
	//Notes: Sets up emitter to the default particle
	//		 effect.
	//////////////////////////////////////////////////////////////
	virtual bool setupEmitter();

	//////////////////////////////////////////////////////////////
	//Func Name: updateEmitter
	//Date: 7/19/05
	//Author: Will Dull
	//Notes: Updates emitter.
	//////////////////////////////////////////////////////////////
	virtual bool updateEmitter();

	virtual ~CDefaultEmitter(void);
};

#endif
