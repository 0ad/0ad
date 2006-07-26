; =========================================================================
; File        : ia32_asm.asm
; Project     : 0 A.D.
; Description : optimized assembly code for IA-32. not provided as
;             : inline assembly because that's compiler-specific.
;
; @author Jan.Wassenberg@stud.uni-karlsruhe.de
; =========================================================================

; Copyright (c) 2004-2005 Jan Wassenberg
;
; Redistribution and/or modification are also permitted under the
; terms of the GNU General Public License as published by th;e
; Free Software Foundation (version 2 or later, at your option).
;
; This program is distributed in the hope that it will be useful, but
; WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

%include "ia32.inc"

;-------------------------------------------------------------------------------
; CPUID support
;-------------------------------------------------------------------------------

[section .data]

; these are actually max_func+1, i.e. the first invalid value.
; the idea here is to avoid a separate cpuid_available flag;
; using signed values doesn't work because ext_funcs are >= 0x80000000.
max_func		dd	0
max_ext_func	dd	0

__SECT__


; extern "C" bool __cdecl ia32_cpuid(u32 func, u32* regs)
global sym(ia32_cpuid)
sym(ia32_cpuid):
	push	ebx
	push	edi

	mov		ecx, [esp+8+4+0]			; func
	mov		edi, [esp+8+4+4]			; -> regs

	; compare against max supported func and fail if above
	xor		eax, eax					; return value on failure
	test	ecx, ecx
	mov		edx, [max_ext_func]
	js		.is_ext_func
	mov		edx, [max_func]
.is_ext_func:
	cmp		ecx, edx
	jae		.ret						; (see max_func decl)

	; issue CPUID and store result registers in array
	mov		eax, ecx
	cpuid
	stosd
	xchg		eax, ebx
	stosd
	xchg		eax, ecx
	stosd
	xchg		eax, edx
	stosd

	; success
	xor		eax, eax
	inc		eax
.ret:
	pop		edi
	pop		ebx
	ret


;-------------------------------------------------------------------------------
; lock-free support routines
;-------------------------------------------------------------------------------

extern sym(cpus)

; extern "C" void __cdecl atomic_add(intptr_t* location, intptr_t increment);
global sym(atomic_add)
sym(atomic_add):
	cmp		byte [sym(cpus)], 1
	mov		edx, [esp+4]				; location
	mov		eax, [esp+8]				; increment
	je		.no_lock
db		0xf0							; LOCK prefix
.no_lock:
	add		[edx], eax
	ret


; notes:
; - this is called via CAS macro, which silently casts its inputs for
;   convenience. mixing up the <expected> and <location> parameters would
;   go unnoticed; we therefore perform a basic sanity check on <location> and
;   raise a warning if it is invalid.
; - a 486 or later processor is required since we use CMPXCHG.
;   there's no feature flag we can check, and the ia32 code doesn't
;   bother detecting anything < Pentium, so this'll crash and burn if
;   run on 386. we could fall back to simple MOVs there (since 386 CPUs
;   aren't MP-capable), but it's not worth the trouble.
; extern "C" __declspec(naked) bool __cdecl CAS_(uintptr_t* location, uintptr_t expected, uintptr_t new_value);
global sym(CAS_)
sym(CAS_):
	cmp		byte [sym(cpus)], 1
	mov		eax, [esp+8]				; expected
	mov		edx, [esp+4]				; location
	cmp		edx, 0x10000				; .. valid pointer?
	jb		.invalid_location			;    no - raise warning
	mov		ecx, [esp+12]				; new_value
	je		.no_lock
db		0xf0							; LOCK prefix
.no_lock:
	cmpxchg	[edx], ecx
	sete	al
	movzx	eax, al
	ret

; NOTE: nasm 0.98.39 doesn't support generating debug info for win32
; output format. that means this code may be misattributed to other
; functions, which makes tracking it down very difficult.
; we therefore raise an "Invalid Opcode" exception, which is rather distinct.
.invalid_location:
	ud2


;-------------------------------------------------------------------------------
; FPU
;-------------------------------------------------------------------------------

; extern "C" uint __cdecl ia32_control87(uint new_cw, uint mask);
global sym(ia32_control87)
sym(ia32_control87):
	push	eax
	fnstcw	[esp]
	pop		eax							; old_cw
	mov		ecx, [esp+4]				; new_val
	mov		edx, [esp+8]				; mask
	and		ecx, edx					; new_val & mask
	not		edx							; ~mask
	and		eax, edx					; old_cw & ~mask
	or		eax, ecx					; (old_cw & ~mask) | (new_val & mask)
	push	eax							; = new_cw
	fldcw	[esp]
	pop		eax
	xor		eax, eax					; return value
	ret


; possible IA-32 FPU control word flags after FXAM: NAN|NORMAL|ZERO
FP_CLASSIFY_MASK	equ 0x4500

; extern "C" uint __cdecl ia32_fpclassify(double d);
global sym(ia32_fpclassify)
sym(ia32_fpclassify):
	fld		qword [esp+4]
	fxam
	fnstsw	ax
	fstp	st0
	and		eax, FP_CLASSIFY_MASK
	ret

; extern "C" uint __cdecl ia32_fpclassifyf(float f);
global sym(ia32_fpclassifyf)
sym(ia32_fpclassifyf):
	fld		dword [esp+4]
	fxam
	fnstsw	ax
	fstp	st0
	and		eax, FP_CLASSIFY_MASK
	ret


;-------------------------------------------------------------------------------
; misc
;-------------------------------------------------------------------------------

; rationale: the common return convention for 64-bit values is in edx:eax.
; with inline asm, we'd have to MOV data to a temporary and return that;
; this is less efficient (-> important for low-overhead profiling) than
; making use of the convention.
;
; however, speed is not the main reason for providing this routine.
; xcode complains about CPUID clobbering ebx, so we use external asm
; where possible (IA-32 CPUs).
;
; extern "C" u64 ia32_rdtsc_edx_eax()
global sym(ia32_rdtsc_edx_eax)
sym(ia32_rdtsc_edx_eax):
	push	ebx
	cpuid
	pop		ebx
	rdtsc
	ret


; write the current execution state (e.g. all register values) into
; (Win32::CONTEXT*)pcontext (defined as void* to avoid dependency).
; optimized for size; this must be straight asm because __declspec(naked)
; is compiler-specific and compiler-generated prolog code inserted before
; inline asm trashes EBP and ESP (unacceptable).
; extern "C" void ia32_get_current_context(void* pcontext)
global sym(ia32_get_current_context)
sym(ia32_get_current_context):
	pushad
	pushfd
	mov		edi, [esp+4+32+4]	; pcontext

	; ContextFlags
	mov		eax, 0x10007		; segs, int, control
	stosd

	; DRx and FloatSave
	; rationale: we can't access the debug registers from Ring3, and
	; the FPU save area is irrelevant, so zero them.
	xor		eax, eax
	push	byte 6+8+20
	pop		ecx
rep	stosd

	; CONTEXT_SEGMENTS
	mov		ax, gs
	stosd
	mov		ax, fs
	stosd
	mov		ax, es
	stosd
	mov		ax, ds
	stosd

	; CONTEXT_INTEGER
	mov		eax, [esp+4+32-32]	; edi
	stosd
	xchg	eax, esi
	stosd
	xchg	eax, ebx
	stosd
	xchg	eax, edx
	stosd
	mov		eax, [esp+4+32-8]	; ecx
	stosd
	mov		eax, [esp+4+32-4]	; eax
	stosd

	; CONTEXT_CONTROL
	xchg	eax, ebp			; ebp restored by POPAD
	stosd
	mov		eax, [esp+4+32]		; return address
	sub		eax, 5				; skip CALL instruction -> call site.
	stosd
	xor		eax, eax
	mov		ax, cs
	stosd
	pop		eax					; eflags
	stosd
	lea		eax, [esp+32+4+4]	; esp
	stosd
	xor		eax, eax
	mov		ax, ss
	stosd

	; ExtendedRegisters
	xor		ecx, ecx
	mov		cl, 512/4
rep	stosd

	popad
	ret


;-------------------------------------------------------------------------------
; init
;-------------------------------------------------------------------------------

; extern "C" bool __cdecl ia32_asm_init()
global sym(ia32_asm_init)
sym(ia32_asm_init):
	push	ebx

	; check if CPUID is supported
	pushfd
	or		byte [esp+2], 32
	popfd
	pushfd
	pop		eax
	xor		edx, edx
	shr		eax, 22						; bit 21 toggled?
	jnc		.no_cpuid

	; determine max supported CPUID function
	xor		eax, eax
	cpuid
	inc		eax							; (see max_func decl)
	mov		[max_func], eax
	mov		eax, 0x80000000
	cpuid
	inc		eax							; (see max_func decl)
	mov		[max_ext_func], eax
.no_cpuid:

	pop		ebx
	ret
