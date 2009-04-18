/**
 * =========================================================================
 * File        : pmt.h
 * Project     : 0 A.D.
 * Description : Timer implementation using ACPI PM timer
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#ifndef INCLUDED_PMT
#define INCLUDED_PMT

static const i64 PMT_FREQ = 3579545;	// (= master oscillator frequency/4)

class ICounter;
extern ICounter* CreateCounterPMT(void* address, size_t size);

#endif	// #ifndef INCLUDED_PMT
