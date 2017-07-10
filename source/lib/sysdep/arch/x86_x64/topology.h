/* Copyright (C) 2011 Wildfire Games.
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
 * detection of CPU and cache topology.
 * thread-safe, no explicit initialization is required.
 */

#ifndef INCLUDED_X86_X64_TOPOLOGY
#define INCLUDED_X86_X64_TOPOLOGY

#include "lib/sysdep/arch/x86_x64/apic.h"	// ApicId

namespace topology {

//-----------------------------------------------------------------------------
// cpu

// the CPU topology, i.e. how many packages, cores and logical processors are
// actually present and enabled, is useful for parameterizing parallel
// algorithms, especially on NUMA systems.
//
// note: OS abstractions usually only mention "processors", which could be
// any mix of the above.

/**
 * @return number of *enabled* CPU packages / sockets.
 **/
LIB_API size_t NumPackages();

/**
 * @return number of *enabled* CPU cores per package.
 * (2 on dual-core systems)
 **/
LIB_API size_t CoresPerPackage();

/**
 * @return number of *enabled* logical processors (aka Hyperthreads)
 * per core. (2 on P4 EE)
 **/
LIB_API size_t LogicalPerCore();

/**
 * @return index of processor package/socket in [0, NumPackages())
 **/
LIB_API size_t PackageFromApicId(ApicId apicId);

/**
 * @return index of processor core in [0, CoresPerPackage())
 **/
LIB_API size_t CoreFromApicId(ApicId apicId);

/**
 * @return index of logical processor in [0, LogicalPerCore())
 **/
LIB_API size_t LogicalFromApicId(ApicId apicId);

/**
 * @param idxPackage, idxCore, idxLogical return values of *FromApicId
 * @return APIC ID (see note at AreApicIdsReliable)
 **/
LIB_API ApicId ApicIdFromIndices(size_t idxPackage, size_t idxCore, size_t idxLogical);


//-----------------------------------------------------------------------------
// L2 cache

// knowledge of the cache topology, i.e. which processors share which caches,
// can be used to reduce contention and increase effective capacity by
// assigning the partner processors to work on the same dataset.
//
// example: Intel Core2 micro-architectures feature L2 caches shared by
// two cores.

/**
 * @return number of distinct L2 caches.
 **/
LIB_API size_t NumCaches();

/**
 * @return L2 cache number (zero-based) to which the given processor belongs.
 **/
LIB_API size_t CacheFromProcessor(size_t processor);

/**
 * @return bit-mask of all processors sharing the given cache.
 **/
LIB_API uintptr_t ProcessorMaskFromCache(size_t cache);

}	// namespace topology

#endif	// #ifndef INCLUDED_X86_X64_TOPOLOGY
