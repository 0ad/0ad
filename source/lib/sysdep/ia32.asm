; set 32-bit attribute once for all sections and activate .text
section .data use32
section .bss use32
section .text use32

; Usage:
; use sym(ia32_cap) instead of _ia32_cap - on relevant platforms, sym() will add
; the underlines automagically, on others it won't
%ifdef DONT_USE_UNDERLINE
%define sym(a) a
%else
%define sym(a) _ %+ a
%endif

;-------------------------------------------------------------------------------
; fast general memcpy
;-------------------------------------------------------------------------------

; optimized for Athlon XP: 7.3% faster (cumulative) than VC7.1's memcpy over
; all 1..64 byte transfer lengths and misalignments. approaches maximum
; mem bandwidth (2000 MiB/s) for transfers >= 192KiB!
; Pentium III performance: about 3% faster in above small buffer benchmark.
;
; disables specialized large transfer (> 64KiB) implementations if SSE
; isn't available; we do assume MMX support, though (quite safe).

; if memcpy size is greater than this,
; .. it's too big for L1. use non-temporal instructions.
UC_THRESHOLD	equ	64*1024
; .. it also blows L2. pull chunks into L1 ("block prefetch").
BP_THRESHOLD	equ	192*1024

; maximum that can be copied by IC_MOVSD.
; if you change this, be sure to expand the movs* table(s)!
IC_SIZE		equ	67

; size of one block prefetch chunk.
; if you change this, make sure "push byte BP_SIZE/128" doesn't overflow!
BP_SIZE		equ	8*1024


; > ecx = size (<= IC_SIZE)
; x eax, ecx
;
; determined to be fastest approach by testing. a movsd table followed by
; rep movsb is a bit smaller but 6.9% slower; everything else is much worse.
%macro IC_MOVSD 0
	mov		eax, ecx
	shr		ecx, 2						; dword count
	neg		ecx
	add		ecx, %%movsd_table_end
	jmp		ecx
align 8
	movsd
	movsd
	movsd
	movsd
	movsd
	movsd
	movsd
	movsd
	movsd
	movsd
	movsd
	movsd
	movsd
	movsd
	movsd
	movsd
%%movsd_table_end:

	and		eax, 3
	neg		eax
	add		eax, %%movsb_table_end
	jmp		eax
	movsb
	movsb
	movsb
%%movsb_table_end:
%endm


; align destination address to multiple of 8.
; not done for small transfers because it doesn't help IC_MOVSD.
%macro IC_ALIGN 0
	mov		eax, 8
	sub		eax, edi
	and		eax, byte 7					; eax = # misaligned bytes
	sub		ecx, eax					; reduce copy count
	neg		eax
	add		eax, %%align_table_end
	jmp		eax
align 4
	movsb
	movsb
	movsb
	movsb
	movsb
	movsb
	movsb
	movsb
%%align_table_end:
%endm


; > ecx = size
; x edx
%macro IC_MOVQ 0
align 16
	mov		edx, 64
%%loop:
	cmp		ecx, edx
	jb		%%done
	prefetchnta	[esi + (200*64/34+192)]
	movq	mm0, [esi+0]
	movq	mm1, [esi+8]
	movq	[edi+0], mm0
	movq	[edi+8], mm1
	movq	mm2, [esi+16]
	movq	mm3, [esi+24]
	movq	[edi+16], mm2
	movq	[edi+24], mm3
	movq	mm0, [esi+32]
	movq	mm1, [esi+40]
	movq	[edi+32], mm0
	movq	[edi+40], mm1
	movq	mm2, [esi+48]
	movq	mm3, [esi+56]
	movq	[edi+48], mm2
	movq	[edi+56], mm3
	add		esi, edx
	add		edi, edx
	sub		ecx, edx
	jmp		%%loop
%%done:
%endm


; > ecx = size (> 64)
; x
%macro UC_MOVNTQ 0
	mov		edx, 64
align 16
%%1:
	prefetchnta [esi + (200*64/34+192)]
	movq	mm0,[esi+0]
	add		edi, edx
	movq	mm1,[esi+8]
	add		esi, edx
	movq	mm2,[esi-48]
	movntq	[edi-64], mm0
	movq	mm0,[esi-40]
	movntq	[edi-56], mm1
	movq	mm1,[esi-32]
	movntq	[edi-48], mm2
	movq	mm2,[esi-24]
	movntq	[edi-40], mm0
	movq	mm0,[esi-16]
	movntq	[edi-32], mm1
	movq	mm1,[esi-8]
	movntq	[edi-24], mm2
	movntq	[edi-16], mm0
	sub		ecx, edx
	movntq	[edi-8], mm1
	cmp		ecx, edx
	jae		%%1
%endm


; > ecx = size (> 8KiB)
; x eax, edx
;
; somewhat optimized for size (futile attempt to avoid near jump)
%macro UC_BP_MOVNTQ 0
%%prefetch_and_copy_chunk:

	; touch each cache line within chunk in reverse order (prevents HW prefetch)
	push	byte BP_SIZE/128			; # iterations
	pop		eax
	add		esi, BP_SIZE
align 8
%%prefetch_chunk:
	mov		edx, [esi-64]
	mov		edx, [esi-128]
	sub		esi, 128
	dec		eax
	jnz		%%prefetch_chunk

	; copy 64 byte blocks
	mov		eax, BP_SIZE/64				; # iterations (> signed 8 bit)
	push	byte 64
	pop		edx
align 8
%%copy_block:
	movq	mm0, [esi+ 0]
	movq	mm1, [esi+ 8]
	movq	mm2, [esi+16]
	movq	mm3, [esi+24]
	movq	mm4, [esi+32]
	movq	mm5, [esi+40]
	movq	mm6, [esi+48]
	movq	mm7, [esi+56]
	add		esi, edx
	movntq	[edi+ 0], mm0
	movntq	[edi+ 8], mm1
	movntq	[edi+16], mm2
	movntq	[edi+24], mm3
	movntq	[edi+32], mm4
	movntq	[edi+40], mm5
	movntq	[edi+48], mm6
	movntq	[edi+56], mm7
	add		edi, edx
	dec		eax
	jnz		%%copy_block

	sub		ecx, BP_SIZE
	cmp		ecx, BP_SIZE
	jae		%%prefetch_and_copy_chunk
%endm


[section .bss]

; this is somewhat "clever". the 2 specialized transfer implementations
; that use SSE are jumped to if transfer size is greater than a threshold.
; we simply set the requested transfer size to 0 if the CPU doesn't
; support SSE so that those are never reached (done by masking with this).
sse_mask		resd	1

__SECT__

; void* __declspec(naked) ia32_memcpy(void* dst, const void* src, size_t nbytes)
; Return dst to make ia32_memcpy usable as a standard library memcpy drop-in
global sym(ia32_memcpy)
sym(ia32_memcpy):
	push	edi
	push	esi

	mov		edi, [esp+8+4+0]			; dst
	mov		esi, [esp+8+4+4]			; src
	mov		ecx, [esp+8+4+8]			; nbytes

	cmp		ecx, byte IC_SIZE
	ja		.choose_larger_method

.ic_movsd:
	IC_MOVSD
	mov		eax, [esp+8+4+0]			; return dst
	pop		esi
	pop		edi
	ret

.choose_larger_method:
	IC_ALIGN

	mov		eax, [sse_mask]
	mov		edx, ecx
	and		edx, eax					; edx = (SSE)? remaining_bytes : 0
	cmp		edx, BP_THRESHOLD
	jae		near .uc_bp_movntq
	cmp		edx, UC_THRESHOLD
	jae		.uc_movntq

.ic_movq:
	IC_MOVQ
	emms
	jmp		.ic_movsd

.uc_movntq:
	UC_MOVNTQ
	sfence
	emms
	jmp		.ic_movsd

.uc_bp_movntq:
	UC_BP_MOVNTQ
	sfence
	jmp		.ic_movq



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
; misc
;-------------------------------------------------------------------------------

; extern "C" uint __cdecl ia32_control87(uint new_cw, uint mask)
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
	
; extern "C" bool __cdecl ia32_init()
global sym(ia32_init)
sym(ia32_init):
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

	; check if SSE is supported (used by memcpy code)
extern sym(ia32_cap)
	push	byte 32+25					; ia32.h's SSE cap (won't change)
	call	sym(ia32_cap)
	pop		edx							; remove stack param
	neg		eax							; SSE? ~0 : 0
	mov		[sse_mask], eax

	pop		ebx
	ret

;-------------------------------------------------------------------------------
; Color conversion (SSE)
;-------------------------------------------------------------------------------

; extern "C" u32 ConvertRGBColorTo4ub(const RGBColor& color)
[section .data]
	align	16
zero:
	dd	0.0
twofivefive:
	dd	255.0


__SECT__
	align	16
global sym(sse_ConvertRGBColorTo4ub)
sym(sse_ConvertRGBColorTo4ub):
	mov	eax, [esp+4]

	; xmm0, 1, 2 = R, G, B
	movss	xmm4, [zero]
	movss	xmm0, [eax+8]
	movss	xmm1, [eax+4]
	movss	xmm2, [eax]
	movss	xmm5, [twofivefive]

	; C = min(255, 255*max(C, 0)) ( == clamp(255*C, 0, 255) )
	maxss	xmm0, xmm4
	maxss	xmm1, xmm4
	maxss	xmm2, xmm4
	mulss	xmm0, xmm5
	mulss	xmm1, xmm5
	mulss	xmm2, xmm5
	minss	xmm0, xmm5
	minss	xmm1, xmm5
	minss	xmm2, xmm5
	
	; convert to integer and combine channels using bit logic
	cvtss2si eax, xmm0
	cvtss2si ecx, xmm1
	cvtss2si edx, xmm2
	shl	eax, 16
	shl	ecx, 8
	or	eax, 0xff000000
	or	edx, ecx
	or	eax, edx

	ret
