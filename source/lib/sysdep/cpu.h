/**
 * =========================================================================
 * File        : cpu.h
 * Project     : 0 A.D.
 * Description : CPU and memory detection.
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#ifndef INCLUDED_CPU
#define INCLUDED_CPU

namespace ERR
{
	const LibError CPU_FEATURE_MISSING     = -130000;
	const LibError CPU_UNKNOWN_OPCODE      = -130001;
	const LibError CPU_UNKNOWN_VENDOR      = -130002;
	
}

//-----------------------------------------------------------------------------
// CPU detection

/**
 * @return string identifying the CPU (usually a cleaned-up version of the
 * brand string)
 **/
LIB_API const char* cpu_IdentifierString();


//-----------------------------------------------------------------------------
// lock-free support routines

/**
 * atomic "compare and swap".
 *
 * @param location address of the word to compare and possibly overwrite
 * @param expected its expected value
 * @param newValue the value with which to replace it
 * @return false if the target word doesn't match the expected value,
 * otherwise true (also overwriting the contents of location)
 **/
LIB_API bool cpu_CAS(volatile uintptr_t* location, uintptr_t expected, uintptr_t newValue);

/**
 * specialization of cpu_CAS for pointer types. this avoids error-prone
 * casting in user code.
 **/
template<typename T>
bool cpu_CAS(volatile T* location, T expected, T new_value)
{
	return cpu_CAS((volatile uintptr_t*)location, (uintptr_t)expected, (uintptr_t)new_value);
}

/**
 * add a signed value to a variable without the possibility of interference
 * from other threads/CPUs.
 **/
LIB_API void cpu_AtomicAdd(volatile intptr_t* location, intptr_t increment);

/**
 * enforce strict instruction ordering in the CPU pipeline.
 **/
LIB_API void cpu_Serialize();

/**
 * enforce strong memory ordering.
 **/
LIB_API void cpu_MemoryFence();


//-----------------------------------------------------------------------------
// misc

/**
 * drop-in replacement for POSIX memcpy().
 **/
LIB_API void* cpu_memcpy(void* RESTRICT dst, const void* RESTRICT src, size_t size);


/**
 * set the FPU control word to "desirable" values (see implementation)
 **/
LIB_API void cpu_ConfigureFloatingPoint();

// convert float to int much faster than _ftol2, which would normally be
// used by (int) casts.
// VC8 and GCC with -ffast-math now manage to generate SSE instructions,
// so our implementation is no longer needed.
#define cpu_i32FromFloat(f) ((i32)f)
#define cpu_i32FromDouble(d) ((i32)d)
#define cpu_i64FromDouble(d) ((i64)d)

#endif	// #ifndef INCLUDED_CPU
