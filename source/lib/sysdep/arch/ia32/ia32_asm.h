/**
 * =========================================================================
 * File        : ia32_asm.h
 * Project     : 0 A.D.
 * Description : interface to various IA-32 functions (written in asm)
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#ifndef INCLUDED_IA32_ASM
#define INCLUDED_IA32_ASM

#ifdef __cplusplus
extern "C" {
#endif

struct x86_x64_CpuidRegs;
extern void CALL_CONV ia32_asm_cpuid(x86_x64_CpuidRegs* regs);

extern void CALL_CONV ia32_asm_AtomicAdd(volatile intptr_t* location, intptr_t increment);
extern bool CALL_CONV ia32_asm_CAS(volatile uintptr_t* location, uintptr_t expected, uintptr_t new_value);

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

/// POSIX rintf
extern float CALL_CONV ia32_asm_rintf(float);
extern double CALL_CONV ia32_asm_rint(double);

/// POSIX fminf
extern float CALL_CONV ia32_asm_fminf(float, float);
extern float CALL_CONV ia32_asm_fmaxf(float, float);

extern i32 CALL_CONV ia32_asm_i32FromFloat(float f);
extern i32 CALL_CONV ia32_asm_i32FromDouble(double d);
extern i64 CALL_CONV ia32_asm_i64FromDouble(double d);

/**
 * write the current execution state (e.g. all register values) into
 * (Win32::CONTEXT*)pcontext (defined as void* to avoid dependency).
 **/
extern void CALL_CONV ia32_asm_GetCurrentContext(void* pcontext);

#ifdef __cplusplus
}
#endif

#endif	// #ifndef INCLUDED_IA32_ASM
