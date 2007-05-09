/**
 * =========================================================================
 * File        : lib.cpp
 * Project     : 0 A.D.
 * Description : various utility functions.
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#include "precompiled.h"
#include "lib.h"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "lib/app_hooks.h"
#include "lib/sysdep/sysdep.h"


u16 addusw(u16 x, u16 y)
{
	u32 t = x;
	return (u16)std::min(t+y, 0xFFFFu);
}

u16 subusw(u16 x, u16 y)
{
	long t = x;
	return (u16)(std::max(t-y, 0l));
}


//-----------------------------------------------------------------------------
// rand

// return random integer in [min, max).
// avoids several common pitfalls; see discussion at
// http://www.azillionmonkeys.com/qed/random.html

// rand() is poorly implemented (e.g. in VC7) and only returns < 16 bits;
// double that amount by concatenating 2 random numbers.
// this is not to fix poor rand() randomness - the number returned will be
// folded down to a much smaller interval anyway. instead, a larger XRAND_MAX
// decreases the probability of having to repeat the loop.
#if RAND_MAX < 65536
static const uint XRAND_MAX = (RAND_MAX+1)*(RAND_MAX+1) - 1;
static uint xrand()
{
	return rand()*(RAND_MAX+1) + rand();
}
// rand() is already ok; no need to do anything.
#else
static const uint XRAND_MAX = RAND_MAX;
static uint xrand()
{
	return rand();
}
#endif

uint rand(uint min_inclusive, uint max_exclusive)
{
	const uint range = (max_exclusive-min_inclusive);
	// huge interval or min >= max
	if(range == 0 || range > XRAND_MAX)
	{
		WARN_ERR(ERR::INVALID_PARAM);
		return 0;
	}

	const uint inv_range = XRAND_MAX / range;

	// generate random number in [0, range)
	// idea: avoid skewed distributions when <range> doesn't evenly divide
	// XRAND_MAX by simply discarding values in the "remainder".
	// not expected to run often since XRAND_MAX is large.
	uint x;
	do
		x = xrand();
	while(x >= range * inv_range);
	x /= inv_range;

	x += min_inclusive;
	debug_assert(x < max_exclusive);
	return x;
}


//-----------------------------------------------------------------------------
// type conversion

// these avoid a common mistake in using >> (ANSI requires shift count be
// less than the bit width of the type).

u32 u64_hi(u64 x)
{
	return (u32)(x >> 32);
}

u32 u64_lo(u64 x)
{
	return (u32)(x & 0xFFFFFFFF);
}

u16 u32_hi(u32 x)
{
	return (u16)(x >> 16);
}

u16 u32_lo(u32 x)
{
	return (u16)(x & 0xFFFF);
}


u64 u64_from_u32(u32 hi, u32 lo)
{
	u64 x = (u64)hi;
	x <<= 32;
	x |= lo;
	return x;
}

u32 u32_from_u16(u16 hi, u16 lo)
{
	u32 x = (u32)hi;
	x <<= 16;
	x |= lo;
	return x;
}


// input in [0, 1); convert to u8 range
u8 u8_from_double(double in)
{
	if(!(0.0 <= in && in < 1.0))
	{
		debug_warn("clampf not in [0,1)");
		return 255;
	}

	int l = (int)(in * 255.0);
	debug_assert((unsigned int)l <= 255u);
	return (u8)l;
}

// input in [0, 1); convert to u16 range
u16 u16_from_double(double in)
{
	if(!(0.0 <= in && in < 1.0))
	{
		debug_warn("clampf not in [0,1)");
		return 65535;
	}

	long l = (long)(in * 65535.0);
	debug_assert((unsigned long)l <= 65535u);
	return (u16)l;
}


//-----------------------------------------------------------------------------
// helpers for module init

void moduleInit_assertCanInit(ModuleInitState init_state)
{
	debug_assert(init_state == MODULE_BEFORE_INIT || init_state == MODULE_SHUTDOWN);
}

void moduleInit_assertInitialized(ModuleInitState init_state)
{
	debug_assert(init_state == MODULE_INITIALIZED);
}

void moduleInit_assertCanShutdown(ModuleInitState init_state)
{
	debug_assert(init_state == MODULE_INITIALIZED);
}

void moduleInit_markInitialized(ModuleInitState* init_state)
{
	*init_state = MODULE_INITIALIZED;
}

void moduleInit_markShutdown(ModuleInitState* init_state)
{
	*init_state = MODULE_SHUTDOWN;
}
