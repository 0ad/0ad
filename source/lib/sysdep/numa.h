/* Copyright (C) 2010 Wildfire Games.
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

#ifndef INCLUDED_NUMA
#define INCLUDED_NUMA

//-----------------------------------------------------------------------------
// node topology

/**
 * @return number of NUMA "nodes" (i.e. groups of CPUs with local memory).
 **/
LIB_API size_t numa_NumNodes();

/**
 * @param processor
 * @return node number (zero-based) to which \<processor\> belongs.
 **/
LIB_API size_t numa_NodeFromProcessor(size_t processor);

/**
 * @param node
 * @return bit-mask of all processors constituting \<node\>.
 **/
LIB_API uintptr_t numa_ProcessorMaskFromNode(size_t node);


//-----------------------------------------------------------------------------
// memory


/**
 * @param node
 * @return bytes of memory available for allocation on \<node\>.
 **/
LIB_API size_t numa_AvailableMemory(size_t node);

/**
 * @return the ratio between maximum and minimum times that one processor
 * from each node required to fill a globally allocated array.
 * in other words, this is the maximum slowdown for NUMA-oblivious
 * memory accesses. Microsoft guidelines require it to be <= 3.
 **/
LIB_API double numa_Factor();

/**
 * @return an indication of whether memory pages are node-interleaved.
 *
 * note: this requires ACPI access, which may not be available on
 * least-permission accounts. the default is to return false so as
 * not to cause callers to panic and trigger performance warnings.
 **/
LIB_API bool numa_IsMemoryInterleaved();

#endif	// #ifndef INCLUDED_NUMA
