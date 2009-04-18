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

static inline u32 get_float_bits(const float x)
{
	u32 ret;
	memcpy(&ret, &x, 4);
	return ret;
}

int floor_log2(const float x)
{
	const u32 i = get_float_bits(x);
	const u32 biased_exp = (i >> 23) & 0xFF;
	return (int)biased_exp - 127;
}
