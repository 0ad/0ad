/**
 * =========================================================================
 * File        : whrt.h
 * Project     : 0 A.D.
 * Description : Windows High Resolution Timer
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#ifndef INCLUDED_WHRT
#define INCLUDED_WHRT

// arranged in decreasing order of preference with values = 0..N-1 so that
// the next best timer can be chosen by incrementing a counter.
//
// rationale for the ordering:
// - TSC must come before QPC and PMT to make sure a bug in the latter on
//   Pentium systems doesn't come up.
// - timeGetTime really isn't as safe as the others, so it should be last.
// - low-overhead and high-resolution tick sources are preferred.
enum WhrtTickSourceId
{
	WHRT_TSC,
	WHRT_HPET,
	WHRT_PMT,
	WHRT_QPC,
	WHRT_TGT,

	WHRT_NUM_TICK_SOURCES
};

enum WhrtOverride
{
	// allow use of a tick source if available and we think it's safe.
	WHRT_DEFAULT = 0,	// (value obviates initialization of overrides[])

	// override our IsSafe decision.
	WHRT_DISABLE,
	WHRT_FORCE
};

extern void whrt_OverrideRecommendation(WhrtTickSourceId id, WhrtOverride override);

extern i64 whrt_Ticks();
extern double whrt_NominalFrequency();
extern double whrt_Resolution();

extern double whrt_Time();

#endif	// #ifndef INCLUDED_WHRT
