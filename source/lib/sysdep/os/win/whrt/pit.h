/**
 * =========================================================================
 * File        : pit.h
 * Project     : 0 A.D.
 * Description : Timer implementation using 82C53/4 PIT
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#ifndef INCLUDED_PIT
#define INCLUDED_PIT

// note: we don't access the PIT for two reasons:
// - it rolls over every 55 ms (1.193 MHz, 16 bit) and would have to be
//   read at least that often, which would require setting high thread
//   priority (dangerous).
// - reading it is slow and cannot be done by two independent users
//   (the second being QPC) since the counter value must be latched.
//
// there are enough other counters anway.

static const i64 PIT_FREQ = 1193182;	// (= master oscillator frequency/12)

#endif	// #ifndef INCLUDED_PIT
