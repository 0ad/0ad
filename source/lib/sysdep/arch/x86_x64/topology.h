/* Copyright (C) 2009 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
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
