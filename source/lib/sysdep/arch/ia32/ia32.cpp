/**
 * =========================================================================
 * File        : ia32.cpp
 * Project     : 0 A.D.
 * Description : routines specific to IA-32
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#include "precompiled.h"

#include "ia32.h"

#include "lib/sysdep/cpu.h"
#include "ia32_memcpy.h"
#include "ia32_asm.h"


LibError ia32_GetCallTarget(void* ret_addr, void** target)
{
	*target = 0;

	// points to end of the CALL instruction (which is of unknown length)
	const u8* c = (const u8*)ret_addr;
	// this would allow for avoiding exceptions when accessing ret_addr
	// close to the beginning of the code segment. it's not currently set
	// because this is really unlikely and not worth the trouble.
	const size_t len = ~0u;

	// CALL rel32 (E8 cd)
	if(len >= 5 && c[-5] == 0xE8)
	{
		*target = (u8*)ret_addr + *(i32*)(c-4);
		return INFO::OK;
	}

	// CALL r/m32 (FF /2)
	// .. CALL [r32 + r32*s]          => FF 14 SIB
	if(len >= 3 && c[-3] == 0xFF && c[-2] == 0x14)
		return INFO::OK;
	// .. CALL [disp32]               => FF 15 disp32
	if(len >= 6 && c[-6] == 0xFF && c[-5] == 0x15)
	{
		void* addr_of_target = *(void**)(c-4);
		// there are no meaningful checks we can perform: we're called from
		// the stack trace code, so ring0 addresses may be legit.
		// even if the pointer is 0, it's better to pass its value on
		// (may help in tracking down memory corruption)
		*target = *(void**)addr_of_target;
		return INFO::OK;
	}
	// .. CALL [r32]                  => FF 00-3F(!14/15)
	if(len >= 2 && c[-2] == 0xFF && c[-1] < 0x40 && c[-1] != 0x14 && c[-1] != 0x15)
		return INFO::OK;
	// .. CALL [r32 + r32*s + disp8]  => FF 54 SIB disp8
	if(len >= 4 && c[-4] == 0xFF && c[-3] == 0x54)
		return INFO::OK;
	// .. CALL [r32 + disp8]          => FF 50-57(!54) disp8
	if(len >= 3 && c[-3] == 0xFF && (c[-2] & 0xF8) == 0x50 && c[-2] != 0x54)
		return INFO::OK;
	// .. CALL [r32 + r32*s + disp32] => FF 94 SIB disp32
	if(len >= 7 && c[-7] == 0xFF && c[-6] == 0x94)
		return INFO::OK;
	// .. CALL [r32 + disp32]         => FF 90-97(!94) disp32
	if(len >= 6 && c[-6] == 0xFF && (c[-5] & 0xF8) == 0x90 && c[-5] != 0x94)
		return INFO::OK;
	// .. CALL r32                    => FF D0-D7                 
	if(len >= 2 && c[-2] == 0xFF && (c[-1] & 0xF8) == 0xD0)
		return INFO::OK;

	WARN_RETURN(ERR::CPU_UNKNOWN_OPCODE);
}


void cpu_ConfigureFloatingPoint()
{
	// no longer set 24 bit (float) precision by default: for
	// very long game uptimes (> 1 day; e.g. dedicated server),
	// we need full precision when calculating the time.
	// if there's a spot where we want to speed up divides|sqrts,
	// we can temporarily change precision there.
	//ia32_asm_control87(IA32_PC_24, IA32_MCW_PC);

	// to help catch bugs, enable as many floating-point exceptions as
	// possible. unfortunately SpiderMonkey triggers all of them.
	// note: passing a flag *disables* that exception.
	ia32_asm_control87(IA32_EM_ZERODIVIDE|IA32_EM_INVALID|IA32_EM_DENORMAL|IA32_EM_OVERFLOW|IA32_EM_UNDERFLOW|IA32_EM_INEXACT, IA32_MCW_EM);

	// no longer round toward zero (truncate). changing this setting
	// resulted in much faster float->int casts, because the compiler
	// could be told (via /QIfist) to use FISTP while still truncating
	// the result as required by ANSI C. however, FPU calculation
	// results were changed significantly, so it had to be disabled.
	//ia32_asm_control87(IA32_RC_CHOP, IA32_MCW_RC);
}


void cpu_AtomicAdd(volatile intptr_t* location, intptr_t increment)
{
	ia32_asm_AtomicAdd(location, increment);
}


bool cpu_CAS(volatile uintptr_t* location, uintptr_t expected, uintptr_t new_value)
{
	return ia32_asm_CAS(location, expected, new_value);
}


void* cpu_memcpy(void* RESTRICT dst, const void* RESTRICT src, size_t size)
{
	return ia32_memcpy(dst, src, size);
}
