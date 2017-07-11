/* Copyright (C) 2010 Wildfire Games.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * pseudorandom number generator
 */

#include "precompiled.h"
#include "lib/rand.h"

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
		WARN_IF_ERR(ERR::INVALID_PARAM);
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
	ENSURE(x < max_exclusive);
	return x;
}
