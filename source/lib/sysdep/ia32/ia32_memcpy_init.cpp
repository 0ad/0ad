/**
 * =========================================================================
 * File        : ia32_memcpy_init.cpp
 * Project     : 0 A.D.
 * Description : initialization for ia32_memcpy (detect CPU caps)
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#include "precompiled.h"

#include "ia32.h"
#include "ia32_memcpy.h"

// set by ia32_memcpy_init, referenced by ia32_memcpy (asm)
// default to "all codepaths supported"
EXTERN_C u32 ia32_memcpy_size_mask = ~0u;

void ia32_memcpy_init()
{
	// set the mask that is applied to transfer size before
	// choosing copy technique. this is the mechanism for disabling
	// codepaths that aren't supported on all CPUs; see article for details.
	// .. check for PREFETCHNTA and MOVNTQ support. these are part of the SSE
	// instruction set, but also supported on older Athlons as part of
	// the extended AMD MMX set.
	if(!ia32_cap(IA32_CAP_SSE) && !ia32_cap(IA32_CAP_AMD_MMX_EXT))
		ia32_memcpy_size_mask = 0u;
}

// ia32_memcpy() is defined in ia32_memcpy_asm.asm
