/**
 * =========================================================================
 * File        : bits.cpp
 * Project     : 0 A.D.
 * Description : bit-twiddling.
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#include "precompiled.h"
#include "bits.h"

int floor_log2(const float x)
{
	const u32 i = *(u32*)&x;
	u32 biased_exp = (i >> 23) & 0xFF;
	return (int)biased_exp - 127;
}
