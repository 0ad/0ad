/**
 * =========================================================================
 * File        : tgt.h
 * Project     : 0 A.D.
 * Description : Timer implementation using timeGetTime
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#ifndef INCLUDED_TGT
#define INCLUDED_TGT

class ICounter;
extern ICounter* CreateCounterTGT(void* address, size_t size);

#endif	// #ifndef INCLUDED_TGT
