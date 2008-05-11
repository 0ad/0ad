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

struct Ia32CpuidRegs;
extern void CALL_CONV ia32_asm_cpuid(Ia32CpuidRegs* regs);

extern void CALL_CONV ia32_asm_AtomicAdd(volatile intptr_t* location, intptr_t increment);
extern bool CALL_CONV ia32_asm_CAS(volatile uintptr_t* location, uintptr_t expected, uintptr_t new_value);


/**
 * for all 1-bits in mask, update the corresponding FPU control word bits
 * with the bit values in new_val.
 * @return 0 to indicate success.
 **/
extern size_t CALL_CONV ia32_asm_control87(size_t new_val, size_t mask);

/// see POSIX fpclassify
extern size_t CALL_CONV ia32_asm_fpclassifyd(double d);
extern size_t CALL_CONV ia32_asm_fpclassifyf(float f);

/// see POSIX rintf
extern float CALL_CONV ia32_asm_rintf(float);
extern double CALL_CONV ia32_asm_rint(double);

/// see POSIX fminf
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
