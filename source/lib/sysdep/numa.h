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

/**
 * @return an indication of whether memory pages are node-interleaved.
 *
 * note: this requires ACPI access, which may not be available on
 * least-permission accounts. the default is to return false so as
 * not to cause callers to panic and trigger performance warnings.
 **/
LIB_API bool numa_IsMemoryInterleaved();


//-----------------------------------------------------------------------------
// allocator

enum LargePageDisposition
{
	LPD_DEFAULT,
	LPD_ALWAYS,
	LPD_NEVER
};

/**
 * simple allocator that "does the right thing" on NUMA systems.
 *
 * @param largePageDisposition - allows forcibly enabling/disabling the use
 * of large pages; the default decision involves a heuristic.
 * @param pageSize if non-zero, receives the size [bytes] of a single page
 * out of those used to map the memory.
 *
 * note: page frames will be taken from the node that first accesses them.
 **/
LIB_API void* numa_Allocate(size_t size, LargePageDisposition largePageDisposition = LPD_DEFAULT, size_t* ppageSize = 0);

/**
 * allocate memory from a specific node.
 *
 * @param node node number (zero-based)
 * @param largePageDisposition - see numa_Allocate
 * @param pageSize - see numa_Allocate
 **/
LIB_API void* numa_AllocateOnNode(size_t node, size_t size, LargePageDisposition largePageDisposition = LPD_DEFAULT, size_t* pageSize = 0);

/**
 * release memory that had been handed out by one of the above allocators.
 **/
LIB_API void numa_Deallocate(void* mem);


#ifdef __cplusplus

template<typename T>
struct numa_Deleter
{
	void operator()(T* p) const
	{
		numa_Deallocate(p);
	}
};

template<typename T>
class numa_Allocator
{
public:
	shared_ptr<T> operator()(size_t size, LargePageDisposition largePageDisposition = LPD_DEFAULT, size_t* ppageSize = 0) const
	{
		return shared_ptr<T>((T*)numa_Allocate(size, largePageDisposition, ppageSize), numa_Deleter<T>());
	}
};

#endif

#endif	// #ifndef INCLUDED_NUMA
