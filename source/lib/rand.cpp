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

/**
 * =========================================================================
 * File        : rand.cpp
 * Project     : 0 A.D.
 * Description : pseudorandom number generator
 * =========================================================================
 */

#include "precompiled.h"
#include "rand.h"

// avoids several common pitfalls; see discussion at
// http://www.azillionmonkeys.com/qed/random.html

// rand() is poorly implemented (e.g. in VC7) and only returns < 16 bits;
// double that amount by concatenating 2 random numbers.
// this is not to fix poor rand() randomness - the number returned will be
// folded down to a much smaller interval anyway. instead, a larger XRAND_MAX
// decreases the probability of having to repeat the loop.
#if RAND_MAX < 65536
static const size_t XRAND_MAX = (RAND_MAX+1)*(RAND_MAX+1) - 1;
static size_t xrand()
{
	return rand()*(RAND_MAX+1) + rand();
}
// rand() is already ok; no need to do anything.
#else
static const size_t XRAND_MAX = RAND_MAX;
static size_t xrand()
{
	return rand();
}
#endif

size_t rand(size_t min_inclusive, size_t max_exclusive)
{
	const size_t range = (max_exclusive-min_inclusive);
	// huge interval or min >= max
	if(range == 0 || range > XRAND_MAX)
	{
		WARN_ERR(ERR::INVALID_PARAM);
		return 0;
	}

	const size_t inv_range = XRAND_MAX / range;

	// generate random number in [0, range)
	// idea: avoid skewed distributions when <range> doesn't evenly divide
	// XRAND_MAX by simply discarding values in the "remainder".
	// not expected to run often since XRAND_MAX is large.
	size_t x;
	do
	{
		x = xrand();
	}
	while(x >= range * inv_range);
	x /= inv_range;

	x += min_inclusive;
	debug_assert(x < max_exclusive);
	return x;
}
