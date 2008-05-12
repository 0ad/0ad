#ifndef INCLUDED_NUMA
#define INCLUDED_NUMA

//-----------------------------------------------------------------------------
// node topology

/**
 * @return number of NUMA "nodes" (i.e. groups of CPUs with local memory).
 **/
LIB_API size_t numa_NumNodes();

/**
 * @return node number (zero-based) to which <processor> belongs.
 **/
LIB_API size_t numa_NodeFromProcessor(size_t processor);

/**
 * @return bit-mask of all processors constituting <node>.
 **/
LIB_API uintptr_t numa_ProcessorMaskFromNode(size_t node);


//-----------------------------------------------------------------------------
// memory

/**
 * @return bytes of memory available for allocation on <node>.
 **/
LIB_API size_t numa_AvailableMemory(size_t node);

/**
 * @return the ratio between maximum and minimum times that one processor
 * from each node required to fill a globally allocated array.
 * in other words, this is the maximum slowdown for NUMA-oblivious
 * memory accesses. Microsoft guidelines require it to be <= 3.
 **/
LIB_API double numa_Factor();


//-----------------------------------------------------------------------------
// allocator

/**
 * simple allocator that "does the right thing" on NUMA systems - page frames
 * will be taken from the node that first accesses them.
 **/
LIB_API void* numa_Allocate(size_t size);

enum LargePageDisposition
{
	LPD_DEFAULT,
	LPD_ALWAYS,
	LPD_NEVER
};

/**
 * allocate memory from a specific node.
 *
 * @param node node number (zero-based)
 * @param largePageDisposition - allows forcibly enabling/disabling the use
 * of large pages; the default decision involves a heuristic.
 * @param pageSize if non-zero, receives the size [bytes] of a single page
 * out of those used to map the memory.
 **/
LIB_API void* numa_AllocateOnNode(size_t size, size_t node, LargePageDisposition largePageDisposition = LPD_DEFAULT, size_t* pageSize = 0);

/**
 * release memory that had been handed out by one of the above allocators.
 **/
LIB_API void numa_Deallocate(void* mem);


#ifdef __cplusplus

// for use with shared_ptr
template<typename T>
struct numa_Deleter
{
	void operator()(T* p) const
	{
		numa_Deallocate(p);
	}
};

#endif

#endif	// #ifndef INCLUDED_NUMA
