; Copyright (c) 2010 Wildfire Games
;
; Permission is hereby granted, free of charge, to any person obtaining
; a copy of this software and associated documentation files (the
; "Software"), to deal in the Software without restriction, including
; without limitation the rights to use, copy, modify, merge, publish,
; distribute, sublicense, and/or sell copies of the Software, and to
; permit persons to whom the Software is furnished to do so, subject to
; the following conditions:
; 
; The above copyright notice and this permission notice shall be included
; in all copies or substantial portions of the Software.
; 
; THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
; EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
; MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
; IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
; CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
; TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
; SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

; assembly code for IA-32 (avoid inline assembly due to differing
; compiler support and/or syntax).

%include "ia32.inc"

;-------------------------------------------------------------------------------
; CPUID support
;-------------------------------------------------------------------------------

; extern "C" void __cdecl ia32_asm_cpuid(x86_x64_CpuidRegs* regs);
global sym(ia32_asm_cpuid)
sym(ia32_asm_cpuid):
	push	ebx							; (clobbered by CPUID)
	push	edi							; (need a register other than eax..edx)

	mov		edi, [esp+8+4]				; -> regs

	mov		eax, [edi+0]				; eax (function)
	mov		ecx, [edi+8]				; ecx (count)
	cpuid
	stosd
	xchg		eax, ebx
	stosd
	xchg		eax, ecx
	stosd
	xchg		eax, edx
	stosd

	pop		edi
	pop		ebx
	ret
