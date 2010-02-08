/* Copyright (c) 2010 Wildfire Games
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
 * detection of CPU and cache topology
 */

#ifndef INCLUDED_TOPOLOGY
#define INCLUDED_TOPOLOGY

// interface rationale:
// - explicit initialization avoids the difficulty and overhead of
//   thread-safe lazy initialization checks.
// - requiring an opaque struct to be passed in ensures users call the
//   init function before using the accessors.
// - delegating responsibility for thread-safety to the caller of the
//   first *_Detect invocation avoids overhead and keeps us independent of
//   the various threading packages (Boost, OpenMP, POSIX, Win32, ..)


//-----------------------------------------------------------------------------
// cpu

/**
 * stores CPU topology, i.e. how many packages, cores and SMT units are
 * actually present and enabled. this is useful for detecting SMP systems,
 * predicting performance and dimensioning thread pools.
 *
 * note: OS abstractions usually only mention "processors", which could be
 * any mix of the above.
 **/
struct CpuTopology;

/**
 * initialize static storage from which topology can be retrieved by
 * means of the following functions.
 * @return const pointer to a shared instance.
 *
 * WARNING: this function must not be reentered before it has returned once.
 **/
LIB_API const CpuTopology* cpu_topology_Detect();

/**
 * @return number of *enabled* CPU packages / sockets.
 **/
LIB_API size_t cpu_topology_NumPackages(const CpuTopology*);

/**
 * @return number of *enabled* CPU cores per package.
 * (2 on dual-core systems)
 **/
LIB_API size_t cpu_topology_CoresPerPackage(const CpuTopology*);

/**
 * @return number of *enabled* hyperthreading units per core.
 * (2 on P4 EE)
 **/
LIB_API size_t cpu_topology_LogicalPerCore(const CpuTopology*);


//-----------------------------------------------------------------------------
// L2 cache

/**
 * stores L2 cache topology, i.e. the mapping between processor and caches.
 * this allows cores sharing a cache to work together on the same dataset,
 * which may reduce contention and increase effective capacity.
 *
 * example: Intel Core2 micro-architectures (e.g. Intel Core2) feature
 * partitioned L2 caches shared by two cores.
 **/
struct CacheTopology;

/**
 * initialize static storage from which topology can be retrieved by
 * means of the following functions.
 * @return const pointer to a shared instance.
 *
 * WARNING: this function must not be reentered before it has returned once.
 **/
LIB_API const CacheTopology* cache_topology_Detect();

/**
 * @return number of distinct L2 caches
 **/
LIB_API size_t cache_topology_NumCaches(const CacheTopology*);

/**
 * @return L2 cache number (zero-based) to which <processor> belongs.
 **/
LIB_API size_t cache_topology_CacheFromProcessor(const CacheTopology*, size_t processor);

/**
 * @return bit-mask of all processors sharing <cache>.
 **/
LIB_API uintptr_t cache_topology_ProcessorMaskFromCache(const CacheTopology*, size_t cache);

#endif	// #ifndef INCLUDED_TOPOLOGY
