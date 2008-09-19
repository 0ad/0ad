/**
 * =========================================================================
 * File        : tsc.h
 * Project     : 0 A.D.
 * Description : Timer implementation using RDTSC
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#ifndef INCLUDED_TSC
#define INCLUDED_TSC

class ICounter;
extern ICounter* CreateCounterTSC(void* address, size_t size);

#endif	// #ifndef INCLUDED_TSC
