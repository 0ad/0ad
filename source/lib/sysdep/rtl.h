/**
 * =========================================================================
 * File        : rtl.h
 * Project     : 0 A.D.
 * Description : functions specific to the toolset's runtime library
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#ifndef INCLUDED_RTL
#define INCLUDED_RTL

LIB_API void* rtl_AllocateAligned(size_t size, size_t alignment);
LIB_API void rtl_FreeAligned(void* alignedPointer);

#endif	// #ifndef INCLUDED_RTL
