; =========================================================================
; File        : amd64_asm.asm
; Project     : 0 A.D.
; Description : 
; =========================================================================

; license: GPL; see lib/license.txt

; For the sym() macro
%include "../ia32/ia32.inc"
; For arg0..3
%include "amd64_abi.inc"

BITS 64

; extern "C" void __cdecl amd64_asm_cpuid(Ia32CpuidRegs* reg);
; reference: http://softwarecommunity.intel.com/articles/eng/2669.htm
global sym(amd64_asm_cpuid)
sym(amd64_asm_cpuid):
	push		rbx			; rbx is the only caller-save register we clobber

	mov			r8, arg0
	mov			eax, DWORD [r8+0]
	mov			ecx, DWORD [r8+8]
	cpuid
	mov			DWORD [r8+0], eax
	mov			DWORD [r8+4], ebx
	mov			DWORD [r8+8], ecx
	mov			DWORD [r8+12], edx

	pop			rbx

	ret
	ALIGN 8

; extern "C" intptr_t amd64_CAS(volatile uintptr_t *location, uintptr_t expected, uintptr_t newValue);
global sym(amd64_CAS)
sym(amd64_CAS):
	mov			rax, arg1 ; expected -> rax
	lock cmpxchg [arg0], arg2
	sete		al
	movzx		rax, al
	ret

; extern "C" void amd64_AtomicAdd(intptr_t *location, intptr_t increment);
global sym(amd64_AtomicAdd)
sym(amd64_AtomicAdd):
	lock add	[arg0], arg1
	ret
