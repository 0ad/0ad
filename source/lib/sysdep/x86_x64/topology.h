/**
 * =========================================================================
 * File        : topology.cpp
 * Project     : 0 A.D.
 * Description : detection of CPU and cache topology
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#ifndef INCLUDED_TOPOLOGY
#define INCLUDED_TOPOLOGY

//-----------------------------------------------------------------------------
// CPU

// OSes typically consider both SMT units and cores to be "processors".
// the following routines determine how many of each are actually present and
// enabled. this information is useful for detecting SMP systems, predicting
// performance and dimensioning thread pools.

/**
 * @return number of *enabled* CPU packages / sockets.
 **/
LIB_API size_t cpu_NumPackages();

/**
 * @return number of *enabled* CPU cores per package.
 * (2 on dual-core systems)
 **/
LIB_API size_t cpu_CoresPerPackage();

/**
 * @return number of *enabled* hyperthreading units per core.
 * (2 on P4 EE)
 **/
LIB_API size_t cpu_LogicalPerCore();


//-----------------------------------------------------------------------------
// L2 cache

// some CPU micro-architectures (e.g. Intel Core2) feature partitioned
// L2 caches. if the cores sharing a cache work together on the same
// sub-problem, contention may be reduced and effective capacity increased.
// the following routines allow discovery of the L2 cache topology:

/**
 * @return number of distinct L2 caches
 **/
LIB_API size_t cpu_NumCaches();

/**
 * @return L2 cache number (zero-based) to which <processor> belongs.
 **/
LIB_API size_t cpu_CacheFromProcessor(size_t processor);

/**
 * @return bit-mask of all processors sharing <cache>.
 **/
LIB_API uintptr_t cpu_ProcessorMaskFromCache(size_t cache);

#endif	// #ifndef INCLUDED_TOPOLOGY
