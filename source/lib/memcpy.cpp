/*
 * block prefetch memcpy for large, uncached arrays
 *
 * src and len must be multiples of CHUNK_SIZE.
 */

#include "precompiled.h"
#include "config.h"

#ifdef HAVE_ASM

void memcpy_nt(void* dst, void* src, int len)
{
__asm
{
	push		esi

	mov			edx, [dst]
	mov			esi, [src]
	mov			ecx, [len]
	shr			ecx, 12						; # chunks
											; smaller than sub	ecx, CHUNK_SIZE below

main_loop:

; prefetch: touch each cache line in chunk
; (backwards to prevent hardware prefetches)
;	add			esi, CHUNK_SIZE
prefetch_loop:
	mov			eax, [esi-64]
	mov			eax, [esi-128]
	sub			esi, 128
	test		esi, 4095					; CHUNK_SIZE-1 (icc doesnt preprocess asm)
	jnz			prefetch_loop


; copy the chunk 64 bytes at a time
write_loop:
	movq		mm0, [esi]
	movq		mm1, [esi+8]
	movq		mm2, [esi+16]
	movq		mm3, [esi+24]
	movq		mm4, [esi+32]
	movq		mm5, [esi+40]
	movq		mm6, [esi+48]
	movq		mm7, [esi+56]
	add			esi, 64
	test		esi, 4095					; CHUNK_SIZE-1
	movntq		[edx], mm0
	movntq		[edx+8], mm1
	movntq		[edx+16], mm2
	movntq		[edx+24], mm3
	movntq		[edx+32], mm4
	movntq		[edx+40], mm5
	movntq		[edx+48], mm6
	movntq		[edx+56], mm7
	lea			edx, [edx+64]				; leave flags intact
	jnz			write_loop

	dec			ecx
	jnz			main_loop

	sfence
	emms

	pop			esi
}
}

#endif	// #ifdef HAVE_ASM