/**
 * =========================================================================
 * File        : amd64_asm.h
 * Project     : 0 A.D.
 * Description : interface to various AMD64 functions (written in asm)
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#ifndef INCLUDED_AMD64_ASM
#define INCLUDED_AMD64_ASM

#ifdef __cplusplus
extern "C" {
#endif

struct x86_x64_CpuidRegs;
extern void CALL_CONV amd64_asm_cpuid(x86_x64_CpuidRegs* reg);

extern intptr_t CALL_CONV amd64_CAS(volatile uintptr_t *location, uintptr_t expected, uintptr_t newValue);

extern void CALL_CONV amd64_AtomicAdd(volatile intptr_t *location, intptr_t increment);

#ifdef __cplusplus
}
#endif

#endif	// #ifndef INCLUDED_AMD64_ASM
