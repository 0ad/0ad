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

struct Ia32CpuidRegs;
extern void CALL_CONV amd64_asm_cpuid(Ia32CpuidRegs* reg);

#ifdef __cplusplus
}
#endif

#endif	// #ifndef INCLUDED_AMD64_ASM
