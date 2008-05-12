/**
 * =========================================================================
 * File        : wcpu.h
 * Project     : 0 A.D.
 * Description : Windows backend of os_cpu
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#ifndef INCLUDED_WCPU
#define INCLUDED_WCPU

#include "win.h"

// "affinity" and "processorNumber" are what Windows sees.
// "processorMask" and "processor" are the idealized representation we expose
// to users. the latter insulates them from process affinity restrictions by
// defining IDs as indices of the nonzero bits within the process affinity.
// these routines are provided for the benefit of wnuma.

extern DWORD_PTR wcpu_AffinityFromProcessorMask(DWORD_PTR processAffinity, uintptr_t processorMask);
extern uintptr_t wcpu_ProcessorMaskFromAffinity(DWORD_PTR processAffinity, DWORD_PTR affinity);

#endif	// #ifndef INCLUDED_WCPU
