; set section attributes
section .data data align=32 use32
section .bss  bss  align=16 use32
section .text code align=64 use32
; activate .text (needs to be separate because __SECT__ will otherwise
; complain that the above definition is redeclaring attributes)
section .text

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

; drop-in replacement for libc memcpy(). only requires CPU support for
; MMX (by now universal). highly optimized for Athlon and Pentium III
; microarchitectures; significantly outperforms VC7.1 memcpy and memcpy_amd.
; for details, see accompanying article.

; if transfer size is at least this much,
; .. it's too big for L1. use non-temporal instructions.
UC_THRESHOLD	equ	64*1024
; .. it also blows L2. pull chunks into L1 ("block prefetch").
BP_THRESHOLD	equ	256*1024

; maximum that can be copied by IC_TINY.
IC_TINY_MAX		equ	63

; size of one block prefetch chunk.
BP_SIZE		equ	8*1024


;------------------------------------------------------------------------------

; [p3] replicating this instead of jumping to it from tailN
; saves 1 clock and costs (7-2)*2 bytes code.
%macro EPILOG 0
	pop		esi
	pop		edi
	mov		eax, [esp+4]		; return dst
	ret
%endm

align 64
tail1:
	mov		al, [esi+ecx*4]
	mov		[edi+ecx*4], al
align 4
tail0:
	EPILOG

align 8
tail3:
	; [p3] 2 reads followed by 2 writes is better than
	; R/W interleaved and RRR/WWW
	mov		al, [esi+ecx*4+2]
	mov		[edi+ecx*4+2], al
; already aligned to 8 due to above code
tail2:
	mov		al, [esi+ecx*4]
	mov		dl, [esi+ecx*4+1]
	mov		[edi+ecx*4], al
	mov		[edi+ecx*4+1], dl
	EPILOG

[section .data]
align 16
tail_table	dd tail0, tail1, tail2, tail3
__SECT__

; 15x unrolled copy loop - transfers DWORDs backwards.
; indexed via table of 8-bit offsets.
; rationale:
; - [p3] backwards vs. forwards makes no difference.
; - MOV is faster than MOVSD.
; - index table is needed because calculating end-6*i is slower than
;   a LUT and we wouldn't want to expand entries to 8 bytes
;   (that'd increase code footprint by 30 bytes)
; - a byte index accessed via MOVZX is better due to less dcache usage.
; - only unrolling 8x and 'reentering' the loop is possible but
;   slower due to fiddling with esi/ecx.
align 64
unrolled_copy_code_start:
%assign i 15
%rep 14	; 15 entries, 1 base case handled below
uc_ %+ i:
    mov     eax, [esi+i*4-4]
    mov     [edi+i*4-4], eax
%assign i i-1
%endrep
; base case: no displacement needed; skip it so that code will
; be aligned to 8 bytes after this.
uc_1:
    mov     eax, [esi]
    mov     [edi], eax
uc_0:
	jmp		[tail_table+edx*4]

[section .data]
align 32
unrolled_copy_index_table:
%assign i 0
%rep 16
	db (uc_ %+ i) - unrolled_copy_code_start
%assign i i+1
%endrep
__SECT__


;------------------------------------------------------------------------------
; tiny copy - handles all cases smaller than IC_MOVQ's 64 byte lower limit.
; > edx = number of bytes (< IC_TINY_MAX)
; < does not return.
; x eax, ecx, edx
%macro IC_TINY 0
	mov		ecx, edx
	shr		ecx, 2
	; calculating this address isn't possible due to skipping displacement on uc1;
	; even so, it'd require calculating -6*ecx, which is slower than LUT.
	movzx	eax, byte [unrolled_copy_index_table+ecx]
	and		edx, byte 3
	add		eax, unrolled_copy_code_start
	jmp		eax
	; never reached! the unrolled loop jumps into tailN, which
	; then returns from the memcpy function.
%endm


;------------------------------------------------------------------------------
; align destination address to multiple of 8. important for large transfers,
; but doesn't affect the tiny technique.
; > esi, edi -> buffers (updated)
; > ecx, edx = transfer size (updated)
; x eax
%macro IC_ALIGN 0
	mov		eax, edi
	and		eax, byte 7					; eax = # misaligned bytes
	jz		already_aligned				; early out
	lea		eax, [align_table_start+eax*2]
	jmp		eax

; [p3] this is no slower than a table of mov and much smaller/simpler
align 8
align_table_start:
%rep 8
	dec		ecx
	movsb
%endrep
	mov		edx, ecx
already_aligned:
%endm


;------------------------------------------------------------------------------
; MMX MOVQ technique. used for in-cache transfers of 64B..64*KiB.
; must run on all CPUs, i.e. cannot use the SSE prefetchnta instruction.
; > ecx = -number_of_bytes (multiple of 64)
; > esi, esi point to end of the buffer, i.e. &last_qword+8.
; < ecx = 0
; x
%macro IC_MOVQ 0

align 16
%%loop:

	; notes:
	; - we can't use prefetch here - this codepath must support all CPUs.
	;   [p3] that makes us 5..15% slower on 1KiB..4KiB transfers.
	; - [p3] simple addressing without +ecx is 3.5% faster.
	; - difference between RR/WW/RR/WW and R..R/W..W:
	;   [p3] none (if simple addressing)
	;   [axp] interleaved is better (with +ecx addressing)
	; - enough time elapses between first and third pair of reads that we
	;   could reuse MM0. there is no performance gain either way and
	;   differing displacements make code compression futile anyway, so
	;   we'll just use MM4..7 for clarity.
	movq	mm0, [esi+ecx]
	movq	mm1, [esi+ecx+8]
	movq	[edi+ecx], mm0
	movq	[edi+ecx+8], mm1
	movq	mm2, [esi+ecx+16]
	movq	mm3, [esi+ecx+24]
	movq	[edi+ecx+16], mm2
	movq	[edi+ecx+24], mm3
	movq	mm4, [esi+ecx+32]
	movq	mm5, [esi+ecx+40]
	movq	[edi+ecx+32], mm4
	movq	[edi+ecx+40], mm5
	movq	mm6, [esi+ecx+48]
	movq	mm7, [esi+ecx+56]
	movq	[edi+ecx+48], mm6
	movq	[edi+ecx+56], mm7
	add		ecx, byte 64
	jnz		%%loop
%endm


;------------------------------------------------------------------------------
; SSE MOVNTQ technique. used for transfers that do not fit in L1,
; i.e. 64KiB..192KiB. requires Pentium III or Athlon; caller checks for this.
; > ecx = -number_of_bytes (multiple of 64)
; > esi, esi point to end of the buffer, i.e. &last_qword+8.
; < ecx = 0
; x
%macro UC_MOVNTQ 0

align 16
%%loop:
	; notes:
	; - the AMD optimization manual recommends prefetch distances according to
	;   (200*BytesPerIter/ClocksPerIter+192), which comes out to ~560 here.
	;   [p3] rounding down to 512 bytes makes for significant gains.
	; - [p3] complex addressing with ecx is 1% faster than adding to esi/edi.
	prefetchnta [esi+ecx+512]
	movq	mm0, [esi+ecx]
	movq	mm1, [esi+ecx+8]
	movq	mm2, [esi+ecx+16]
	movq	mm3, [esi+ecx+24]
	movq	mm4, [esi+ecx+32]
	movq	mm5, [esi+ecx+40]
	movq	mm6, [esi+ecx+48]
	movq	mm7, [esi+ecx+56]
	movntq	[edi+ecx], mm0
	movntq	[edi+ecx+8], mm1
	movntq	[edi+ecx+16], mm2
	movntq	[edi+ecx+24], mm3
	movntq	[edi+ecx+32], mm4
	movntq	[edi+ecx+40], mm5
	movntq	[edi+ecx+48], mm6
	movntq	[edi+ecx+56], mm7
	add		ecx, byte 64
	jnz		%%loop
%endm


;------------------------------------------------------------------------------
; block prefetch technique. used for transfers that do not fit in L2,
; i.e. > 192KiB. requires Pentium III or Athlon; caller checks for this.
; for theory behind this, see article.
; > ecx = -number_of_bytes (multiple of 64, <= -BP_SIZE)
; > esi, esi point to end of the buffer, i.e. &last_qword+8.
; < ecx = -remaining_bytes (multiple of 64, > -BP_SIZE)
; < eax = 0
%macro UC_BP_MOVNTQ 0
	push	edx

align 4
%%prefetch_and_copy_chunk:
	; pull chunk into cache by touching each cache line
	; (in reverse order to prevent HW prefetches)
	mov		eax, BP_SIZE/128			; # iterations
	add		esi, BP_SIZE
align 16
%%prefetch_loop:
	mov		edx, [esi+ecx-64]
	mov		edx, [esi+ecx-128]
	add		esi, byte -128
	dec		eax
	jnz		%%prefetch_loop

	; copy chunk in 64 byte pieces
	mov		eax, BP_SIZE/64				; # iterations (> signed 8 bit)
align 16
%%copy_loop:
	movq	mm0, [esi+ecx]
	movq	mm1, [esi+ecx+8]
	movq	mm2, [esi+ecx+16]
	movq	mm3, [esi+ecx+24]
	movq	mm4, [esi+ecx+32]
	movq	mm5, [esi+ecx+40]
	movq	mm6, [esi+ecx+48]
	movq	mm7, [esi+ecx+56]
	movntq	[edi+ecx], mm0
	movntq	[edi+ecx+8], mm1
	movntq	[edi+ecx+16], mm2
	movntq	[edi+ecx+24], mm3
	movntq	[edi+ecx+32], mm4
	movntq	[edi+ecx+40], mm5
	movntq	[edi+ecx+48], mm6
	movntq	[edi+ecx+56], mm7

	add		ecx, byte 64
	dec		eax
	jnz		%%copy_loop

	; if enough data left, process next chunk
	cmp		ecx, -BP_SIZE
	jle		%%prefetch_and_copy_chunk

	pop		edx
%endm


;------------------------------------------------------------------------------

; void* __declspec(naked) ia32_memcpy(void* dst, const void* src, size_t nbytes)
; drop-in replacement for libc memcpy() (returns dst)
global sym(ia32_memcpy)
align 64
sym(ia32_memcpy):
	push	edi
	push	esi

	mov		ecx, [esp+8+4+8]			; nbytes
	mov		edi, [esp+8+4+0]			; dst
	mov		esi, [esp+8+4+4]			; src

	mov		edx, ecx
	cmp		ecx, byte IC_TINY_MAX
	ja		choose_larger_method

ic_tiny:
	IC_TINY
	; never reached - IC_TINY contains memcpy function epilog code

choose_larger_method:
	IC_ALIGN

	; setup:
	; eax = number of 64 byte chunks, or 0 if CPU doesn't support SSE.
	;       used to choose copy technique.
	; ecx = -number_of_bytes, multiple of 64. we jump to ic_tiny if
	;       there's not enough left for a single 64 byte chunk, which can
	;       happen on unaligned 64..71 byte transfers due to IC_ALIGN.
	; edx = number of remainder bytes after qwords have been copied;
	;       will be handled by IC_TINY.
	; esi and edi point to end of the respective buffers (more precisely,
	;       to buffer_start-ecx). this together with the ecx convention means
	;       we only need one loop counter (instead of having to advance
	;       that and esi/edi).

	; this mask is applied to the transfer size. the 2 specialized copy techniques
	; that use SSE are jumped to if size is greater than a threshold.
	; we simply set the requested transfer size to 0 if the CPU doesn't
	; support SSE so that those are never reached (done by masking with this).
	extern sym(ia32_memcpy_size_mask)
	mov		eax, [sym(ia32_memcpy_size_mask)]
	and		ecx, byte ~IC_TINY_MAX
	jz		ic_tiny						; < 64 bytes left (due to IC_ALIGN)
	add		esi, ecx
	add		edi, ecx
	and		edx, byte IC_TINY_MAX
	and		eax, ecx
	neg		ecx

	cmp		eax, BP_THRESHOLD
	jae		near uc_bp_movntq
	cmp		eax, UC_THRESHOLD
	jae		uc_movntq

ic_movq:
	IC_MOVQ
	emms
	jmp		ic_tiny

uc_movntq:
	UC_MOVNTQ
	sfence
	emms
	jmp		ic_tiny

uc_bp_movntq:
	UC_BP_MOVNTQ
	sfence
	cmp		ecx, byte -(IC_TINY_MAX+1)
	jle		ic_movq
	emms
	jmp		ic_tiny


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
