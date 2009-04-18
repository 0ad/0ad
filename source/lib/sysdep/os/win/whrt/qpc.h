/**
 * =========================================================================
 * File        : qpc.h
 * Project     : 0 A.D.
 * Description : Timer implementation using QueryPerformanceCounter
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#ifndef INCLUDED_QPC
#define INCLUDED_QPC

class ICounter;
extern ICounter* CreateCounterQPC(void* address, size_t size);

#endif	// #ifndef INCLUDED_QPC
