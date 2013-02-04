/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FMRandom.h"

// Important: Platform notes
//-------------------
// Windows: the pseudo-random number generator never exceeds 0x7FFF.

namespace FMRandom
{
	void Seed(uint32 seed) 
	{
		srand(seed);
		rand(); // The first number is usually crap, skip it.
	}

	uint32 GetUInt32()
	{
		return rand() & 0x7FFF;
	}

	int32 GetInt32()
	{
		return (rand() & 0x7FFF) - 0x3FFF;
	}

	float GetFloat()
	{
		return ((float) (rand() & 0x7FFF)) / ((float) 0x7FFF);
	}
};

