/* Copyright (c) 2010 Wildfire Games
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

/*
 * routines specific to IA-32
 */

#include "precompiled.h"

#include "lib/sysdep/arch/ia32/ia32.h"

#include "lib/sysdep/cpu.h"
#include "lib/sysdep/arch/ia32/ia32_asm.h"


static const size_t maxInstructionLength = 15;	// IA-32 limitation

static bool IsCall(void* ret_addr, void*& target)
{
	target = 0;	// (not always possible to determine)

	// points to end of the CALL instruction (which is of unknown length)
	const u8* c = (const u8*)ret_addr;
	// this would allow for avoiding exceptions when accessing ret_addr
	// close to the beginning of the code segment. it's not currently set
	// because this is really unlikely and not worth the trouble.
	const size_t len = maxInstructionLength;

	// (most frequent cases first to reduce stack walk overhead:)

	// CALL rel32 (E8 cd)
	if(len >= 5 && c[-5] == 0xE8)
	{
		i32 offset;
		memcpy(&offset, c-sizeof(offset), sizeof(offset));
		target = (void*)(intptr_t(ret_addr) + intptr_t(offset));
		return true;
	}

	// CALL r32                    => FF D0-D7                 
	if(len >= 2 && c[-2] == 0xFF && (c[-1] & 0xF8) == 0xD0)
		return true;

	// CALL [r32 + disp8]          => FF 50-57(!54) disp8
	if(len >= 3 && c[-3] == 0xFF && (c[-2] & 0xF8) == 0x50 && c[-2] != 0x54)
		return true;

	// CALL [disp32]               => FF 15 disp32
	if(len >= 6 && c[-6] == 0xFF && c[-5] == 0x15)
	{
		void** addr_of_target;
		memcpy(&addr_of_target, c-sizeof(addr_of_target), sizeof(addr_of_target));
		target = *addr_of_target;
		// there are no meaningful checks we can perform: we're called from
		// the stack trace code, so ring0 addresses may be legit.
		// even if the pointer is 0, it's better to pass its value on
		// (may help in tracking down memory corruption)
		return true;
	}

	// CALL [r32 + r32*s]          => FF 14 SIB
	if(len >= 3 && c[-3] == 0xFF && c[-2] == 0x14)
		return true;
	// CALL [r32]                  => FF 00-3F(!14/15)
	if(len >= 2 && c[-2] == 0xFF && c[-1] < 0x40 && c[-1] != 0x14 && c[-1] != 0x15)
		return true;
	// CALL [r32 + r32*s + disp8]  => FF 54 SIB disp8
	if(len >= 4 && c[-4] == 0xFF && c[-3] == 0x54)
		return true;
	// CALL [r32 + r32*s + disp32] => FF 94 SIB disp32
	if(len >= 7 && c[-7] == 0xFF && c[-6] == 0x94)
		return true;
	// CALL [r32 + disp32]         => FF 90-97(!94) disp32
	if(len >= 6 && c[-6] == 0xFF && (c[-5] & 0xF8) == 0x90 && c[-5] != 0x94)
		return true;

	return false;
}

LibError ia32_GetCallTarget(void* ret_addr, void*& target)
{
	if(IsCall(ret_addr, target))
	{
		// follow the incremental linker's jump tables
		const u8* c = (const u8*)target;
		if(c && c[0] == 0xE9)
		{
			i32 offset;
			memcpy(&offset, c+1, sizeof(offset));
			target = (void*)(intptr_t(c)+5 + intptr_t(offset));
		}

		return INFO::OK;
	}

	const u8* const instructionWindow = (const u8*)ret_addr - maxInstructionLength;
	if(memchr(instructionWindow, 0xCC, maxInstructionLength))
		return ERR::AGAIN;	// NOWARN (debugger has inserted a breakpoint)

	// this shouldn't normally be reached but might happen if the
	// call stack is corrupted. note that we mustn't warn, because
	// this routine is already called from the stack dump chain.
	debug_break();
	return ERR::CPU_UNKNOWN_OPCODE;	// NOWARN (see above)
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
