CACHEBLOCK equ 128
BP_MIN_THRESHOLD_64 equ 192*1024
MOVNTQ_MIN_THRESHOLD_64 equ 64*1024

section .text

%macro MC_UNROLLED_MOVSD 0
	and	ebx, 63
	mov	edx, ebx
	shr	edx, 2			; dword count
	neg	edx
	add	edx, %%movsd_table_end
	jmp	edx

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

	mov	eax, ebx
	and	eax, 3
	neg	eax
	add	eax, %%movsb_table_end
	jmp	eax

	movsb
	movsb
	movsb
%%movsb_table_end:
%endm


%macro MC_ALIGN 0
	mov		eax, 8
	sub		eax, edi
	and		eax, 7
	cmp		eax, ecx
	cmova		eax, ecx
	sub		ecx, eax
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


%macro MC_MOVQ 0
align 16
%%1:
	prefetchnta	[esi + (200*64/34+192)]
	movq		mm0, [esi+0]
	movq		mm1, [esi+8]
	movq		[edi+0], mm0
	movq		[edi+8], mm1
	movq		mm2, [esi+16]
	movq		mm3, [esi+24]
	movq		[edi+16], mm2
	movq		[edi+24], mm3
	movq		mm0, [esi+32]
	movq		mm1, [esi+40]
	movq		[edi+32], mm0
	movq		[edi+40], mm1
	movq		mm2, [esi+48]
	movq		mm3, [esi+56]
	movq		[edi+48], mm2
	movq		[edi+56], mm3
	add		esi, 64
	add		edi, 64
	dec		ecx
	jnz		%%1
%endm


; we have >= 8kb. until no more 8kb blocks
%macro MC_BP_MOVNTQ 0
%%prefetch_and_copy_chunk:
	mov		eax, CACHEBLOCK / 2		; block prefetch loop, unrolled 2X
	add		esi, CACHEBLOCK * 64	; move to the top of the block
align 16
	; touch each cache line in reverse order (prevents HW prefetch)
%%prefetch_chunk:
	mov		edx, [esi-64]
	mov		edx, [esi-128]
	sub		esi, 128
	dec		eax
	jnz		%%prefetch_chunk
	mov		eax, CACHEBLOCK		; now that it's in cache, do the copy
align 16
%%copy_block:
	movq		mm0, [esi+ 0]
	movq		mm1, [esi+ 8]
	movq		mm2, [esi+16]
	movq		mm3, [esi+24]
	movq		mm4, [esi+32]
	movq		mm5, [esi+40]
	movq		mm6, [esi+48]
	movq		mm7, [esi+56]
	add		esi, 64
	movntq		[edi+ 0], mm0
	movntq		[edi+ 8], mm1
	movntq		[edi+16], mm2
	movntq		[edi+24], mm3
	movntq		[edi+32], mm4
	movntq		[edi+40], mm5
	movntq		[edi+48], mm6
	movntq		[edi+56], mm7
	add		edi, 64
	dec		eax
	jnz		%%copy_block
	sub		ecx, CACHEBLOCK		; update the 64-byte block count
	cmp		ecx, CACHEBLOCK
	jl		%%prefetch_and_copy_chunk
%endm


; we have >= 64, 64B BLOCKS
%macro MC_MOVNTQ 0
align 16
%%1:
	prefetchnta [esi + (200*64/34+192)]
	movq		mm0,[esi+0]
	add		edi,64
	movq		mm1,[esi+8]
	add		esi,64
	movq		mm2,[esi-48]
	movntq		[edi-64], mm0
	movq		mm0,[esi-40]
	movntq		[edi-56], mm1
	movq		mm1,[esi-32]
	movntq		[edi-48], mm2
	movq		mm2,[esi-24]
	movntq		[edi-40], mm0
	movq		mm0,[esi-16]
	movntq		[edi-32], mm1
	movq		mm1,[esi-8]
	movntq		[edi-24], mm2
	movntq		[edi-16], mm0
	dec		ecx
	movntq		[edi-8], mm1
	jnz		%%1
%endm








; void __declspec(naked) ia32_memcpy(void* dst, const void* src, size_t nbytes)
	mov	ecx, [esp+4+8]		; nbytes
	mov	esi, [esp+4+4]		; src
	mov	edi, [esp+4+0]		; dst

	MC_ALIGN

	mov	ebx, ecx
	shr	ecx, 6			; # blocks

	mov	eax, _bp
	cmp	ecx, BP_MIN_THRESHOLD_64
	mov	edx, _movntq
	cmovb	eax, edx
	cmp	ecx, MOVNTQ_MIN_THRESHOLD_64
	mov	edx, _mmx
	cmovb	eax, edx
	cmp	ecx, 64
	jbe	tiny
	jmp	eax

tiny:
	MC_UNROLLED_MOVSD
	ret

_mmx:
	MC_MOVQ
	emms
	jmp		tiny

_bp:
	MC_BP_MOVNTQ
	sfence
	emms
	; protect routine below
	cmp		ecx, 0
	jz		tiny

_movntq:

	sfence
	emms
	jmp		tiny






















; extern "C" int __cdecl get_cur_processor_id();
global _get_cur_processor_id
_get_cur_processor_id:
	push		ebx
	push		1
	pop		eax
	cpuid
	shr		ebx, 24
	mov		eax, ebx					; ebx[31:24]
	pop		ebx





; extern "C" uint __cdecl ia32_control87(uint new_cw, uint mask)
global _ia32_control87
_ia32_control87:
	push		eax
	fnstcw		[esp]
	pop		eax								; old_cw
	mov		ecx, [esp+4]					; new_cw
	mov		edx, [esp+8]					; mask
	and		ecx, edx						; new_cw & mask
	not		edx								; ~mask
	and		eax, edx						; old_cw & ~mask
	or		eax, ecx						; (old_cw & ~mask) | (new_cw & mask)
	push		edx
	fldcw		[esp]
	pop		edx
	xor		eax, eax					; return value