#include "ia32.h"
#include "types.h"
#include "misc.h"


#ifndef _M_IX86
#error "#define _M_IX86 to enable IA-32 code"
#endif


inline u64 rdtsc()
{
	u64 c;
__asm
{
	cpuid
	rdtsc
	mov		dword ptr [c], eax
	mov		dword ptr [c+4], edx
}
	// 64 bit values are returned in edx:eax, but we do it the safe way
	return c;
}


// change FPU control word (used to set precision)
uint _control87(uint new_cw, uint mask)
{
__asm
{
	push	eax
	fnstcw	[esp]
	pop		eax					; old_cw
	mov		ecx, [new_cw]
	mov		edx, [mask]
	and		ecx, edx			; new_cw & mask
	not		edx					; ~mask
	and		eax, edx			; old_cw & ~mask
	or		eax, ecx			; (old_cw & ~mask) | (new_cw & mask)
	push	eax
	fldcw	[esp]
	pop		eax
}

	UNUSED(new_cw)
	UNUSED(mask)

	return 0;
}
