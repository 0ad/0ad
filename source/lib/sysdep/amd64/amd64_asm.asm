; =========================================================================
; File        : amd64_asm.asm
; Project     : 0 A.D.
; Description : 
; =========================================================================

; license: GPL; see lib/license.txt

; extern "C" void __cdecl amd64_asm_cpuid(Ia32CpuidRegs* reg);
; reference: http://softwarecommunity.intel.com/articles/eng/2669.htm
PUBLIC amd64_asm_cpuid
.CODE
	ALIGN 8
amd64_asm_cpuid	PROC FRAME
	sub			rsp, 32
	.allocstack 32
	push		rbx
	.pushreg rbx
	.endprolog

	mov			r8, rcx
	mov			eax, DWORD PTR [r8+0]
	mov			ecx, DWORD PTR [r8+8]
	cpuid
	mov			DWORD PTR [r8+0], eax
	mov			DWORD PTR [r8+4], ebx
	mov			DWORD PTR [r8+8], ecx
	mov			DWORD PTR [r8+12], edx

	pop			rbx
	add			rsp, 32

	ret
	ALIGN 8
cpuid64 ENDP
_TEXT ENDS
