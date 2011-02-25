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
 * interface to various IA-32 functions (written in assembly language)
 */

#ifndef INCLUDED_IA32_ASM
#define INCLUDED_IA32_ASM

#ifdef __cplusplus
extern "C" {
#endif

struct x86_x64_CpuidRegs;
extern void CALL_CONV ia32_asm_cpuid(x86_x64_CpuidRegs* regs);

/// control87
// FPU control word
// .. Precision Control:
const u32 IA32_MCW_PC = 0x0300;
const u32 IA32_PC_24  = 0x0000;
// .. Rounding Control:
const u32 IA32_MCW_RC  = 0x0C00;
const u32 IA32_RC_NEAR = 0x0000;
const u32 IA32_RC_DOWN = 0x0400;
const u32 IA32_RC_UP   = 0x0800;
const u32 IA32_RC_CHOP = 0x0C00;
// .. Exception Mask:
const u32 IA32_MCW_EM        = 0x3F;
const u32 IA32_EM_INVALID    = 0x01;
const u32 IA32_EM_DENORMAL   = 0x02;
const u32 IA32_EM_ZERODIVIDE = 0x04;
const u32 IA32_EM_OVERFLOW   = 0x08;
const u32 IA32_EM_UNDERFLOW  = 0x10;
const u32 IA32_EM_INEXACT    = 0x20;
/**
 * for all 1-bits in mask, update the corresponding FPU control word bits
 * with the bit values in new_val.
 * @return 0 to indicate success.
 **/
extern u32 CALL_CONV ia32_asm_control87(u32 new_val, u32 mask);

/// POSIX fpclassify
#define IA32_FP_NAN       0x0100
#define IA32_FP_NORMAL    0x0400
#define IA32_FP_INFINITE  (IA32_FP_NAN | IA32_FP_NORMAL)
#define IA32_FP_ZERO      0x4000
#define IA32_FP_SUBNORMAL (IA32_FP_NORMAL | IA32_FP_ZERO)
extern size_t CALL_CONV ia32_asm_fpclassifyd(double d);
extern size_t CALL_CONV ia32_asm_fpclassifyf(float f);

/**
 * write the current execution state (e.g. all register values) into
 * (Win32::CONTEXT*)pcontext (defined as void* to avoid dependency).
 **/
extern void CALL_CONV ia32_asm_GetCurrentContext(void* pcontext);

#ifdef __cplusplus
}
#endif

#endif	// #ifndef INCLUDED_IA32_ASM
