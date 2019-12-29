// This code is in the public domain -- castanyo@yahoo.es

#pragma once
#ifndef NV_THREAD_H
#define NV_THREAD_H

#include "nvcore/nvcore.h"

// Function linkage
#if NVTHREAD_SHARED
#ifdef NVTHREAD_EXPORTS
#define NVTHREAD_API DLL_EXPORT
#define NVTHREAD_CLASS DLL_EXPORT_CLASS
#else
#define NVTHREAD_API DLL_IMPORT
#define NVTHREAD_CLASS DLL_IMPORT
#endif
#else // NVMATH_SHARED
#define NVTHREAD_API
#define NVTHREAD_CLASS
#endif // NVMATH_SHARED


// Compiler barriers.
// See: http://en.wikipedia.org/wiki/Memory_ordering
#if NV_CC_MSVC

#include <intrin.h>

#pragma intrinsic(_WriteBarrier)
#define nvCompilerWriteBarrier      _WriteBarrier

#pragma intrinsic(_ReadWriteBarrier)
#define nvCompilerReadWriteBarrier  _ReadWriteBarrier

#if _MSC_VER >= 1400            // ReadBarrier is VC2005
#pragma intrinsic(_ReadBarrier)
#define nvCompilerReadBarrier       _ReadBarrier	
#else
#define nvCompilerReadBarrier       _ReadWriteBarrier
#endif

#elif NV_CC_GNUC

#define nvCompilerReadWriteBarrier()    asm volatile("" ::: "memory");
#define nvCompilerWriteBarrier          nvCompilerReadWriteBarrier
#define nvCompilerReadBarrier           nvCompilerReadWriteBarrier

#elif NV_CC_CLANG && NV_CPU_ARM
// thanks to Autor Artur Bac for 
inline void sync_synchronize() { asm volatile( "dmb;"); }

/* this is not yet supported by LLVM 2.1 but it is planned
#define nvCompilerReadWriteBarrier()    MemoryFence()
 */


// JBeilin: from what i read this should do the trick for ARM 
// however this might also be wrong and dumb.
//#define nvCompilerReadWriteBarrier()    asm volatile( "dmb;");
#define nvCompilerReadWriteBarrier()    nvCompilerReadWriteBarrier()
#define nvCompilerWriteBarrier          nvCompilerReadWriteBarrier
#define nvCompilerReadBarrier           nvCompilerReadWriteBarrier


#endif // NV_CC_MSVC


// @@ Memory barriers / fences.

// @@ Atomics.


/* Wrap this up:
#define YieldProcessor() __asm { rep nop }
#define YieldProcessor _mm_pause
#define YieldProcessor __yield

BOOL WINAPI SwitchToThread(void);
*/


namespace nv
{
    //void initThreadingSystemInfo();

    // Reentrant.
    uint processorCount();
    uint logicalProcessorCount();
    uint physicalProcessorCount();

    // Locks the current thread to the given logical processor index.
    void lockThreadToProcessor(int idx);
    void unlockThreadToProcessor();

    uint threadId();

} // nv namespace






#endif // NV_THREAD_H
