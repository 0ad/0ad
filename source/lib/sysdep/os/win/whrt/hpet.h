/**
 * =========================================================================
 * File        : hpet.h
 * Project     : 0 A.D.
 * Description : Timer implementation using High Precision Event Timer
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#ifndef INCLUDED_HPET
#define INCLUDED_HPET

class ICounter;
extern ICounter* CreateCounterHPET(void* address, size_t size);

#endif	// #ifndef INCLUDED_HPET
