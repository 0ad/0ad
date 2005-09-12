// IA-32 (x86) specific code
// Copyright (c) 2003 Jan Wassenberg
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// Contact info:
//   Jan.Wassenberg@stud.uni-karlsruhe.de
//   http://www.stud.uni-karlsruhe.de/~urkt/

#include "precompiled.h"

#include "lib.h"
#include "posix.h"
#include "ia32.h"
#include "detect.h"
#include "timer.h"

// HACK (see call to wtime_reset_impl)
#if OS_WIN
#include "win/wtime.h"
#endif


#include <string.h>
#include <stdio.h>

#include <vector>
#include <algorithm>

#if HAVE_ASM

// replace pathetic MS libc implementation.
#if OS_WIN
double _ceil(double f)
{
	double r;

	const float _49 = 0.499999f;
__asm
{
	fld			[f]
	fadd		[_49]
	frndint
	fstp		[r]
}

	UNUSED2(f);

	return r;
}
#endif


// return convention for 64 bits with VC7.1, ICC8 is in edx:eax,
// so temp variable is unnecessary, but we play it safe.
inline u64 rdtsc()
{
	u64 c;
__asm
{
	cpuid
	rdtsc
	mov			dword ptr [c], eax
	mov			dword ptr [c+4], edx
}
	return c;
}


// change FPU control word (used to set precision)
uint ia32_control87(uint new_cw, uint mask)
{
__asm
{
	push		eax
	fnstcw		[esp]
	pop			eax								; old_cw
	mov			ecx, [new_cw]
	mov			edx, [mask]
	and			ecx, edx						; new_cw & mask
	not			edx								; ~mask
	and			eax, edx						; old_cw & ~mask
	or			eax, ecx						; (old_cw & ~mask) | (new_cw & mask)
	push		eax
	fldcw		[esp]
	pop			eax
}

	UNUSED2(new_cw);
	UNUSED2(mask);

	return 0;
}


void ia32_debug_break()
{
	__asm int 3
}



/*

conclusions:

for small (<= 64 byte) memcpy, a table of 16 movsd followed by table of 3 movsb is fastest

dtable_btable:     434176
dtable_brep:       464001  6.9
memcpy:            499936 15.1



*/






// Very optimized memcpy() routine for all AMD Athlon and Duron family.
// This code uses any of FOUR different basic copy methods, depending
// on the transfer size.
// NOTE:  Since this code uses MOVNTQ (also known as "Non-Temporal MOV" or
// "Streaming Store"), and also uses the software prefetchnta instructions,
// be sure you're running on Athlon/Duron or other recent CPU before calling!

#define TINY_BLOCK_COPY 64       // upper limit for movsd type copy
// The smallest copy uses the X86 "movsd" instruction, in an optimized
// form which is an "unrolled loop".

#define IN_CACHE_COPY 64 * 1024  // upper limit for movq/movq copy w/SW prefetch
// Next is a copy that uses the MMX registers to copy 8 bytes at a time,
// also using the "unrolled loop" optimization.   This code uses
// the software prefetch instruction to get the data into the cache.

#define UNCACHED_COPY 197 * 1024 // upper limit for movq/movntq w/SW prefetch
// For larger blocks, which will spill beyond the cache, it's faster to
// use the Streaming Store instruction MOVNTQ.   This write instruction
// bypasses the cache and writes straight to main memory.  This code also
// uses the software prefetch instruction to pre-read the data.
// USE 64 * 1024 FOR THIS VALUE IF YOU'RE ALWAYS FILLING A "CLEAN CACHE"

#define BLOCK_PREFETCH_COPY  infinity // no limit for movq/movntq w/block prefetch 
#define CACHEBLOCK 80h // number of 64-byte blocks (cache lines) for block prefetch
// For the largest size blocks, a special technique called Block Prefetch
// can be used to accelerate the read operations.   Block Prefetch reads
// one address per cache line, for a series of cache lines, in a short loop.
// This is faster than using software prefetch.  The technique is great for
// getting maximum read bandwidth, especially in DDR memory systems.



void org_memcpy_amd(u8 *dest, const u8 *src, size_t n)
{
  __asm {

	mov		ecx, [n]		; number of bytes to copy
	mov		edi, [dest]		; destination
	mov		esi, [src]		; source
	mov		ebx, ecx		; keep a copy of count

	cld
	cmp		ecx, TINY_BLOCK_COPY
	jb		$memcpy_ic_3	; tiny? skip mmx copy

	cmp		ecx, 32*1024		; don't align between 32k-64k because
	jbe		$memcpy_do_align	;  it appears to be slower
	cmp		ecx, 64*1024
	jbe		$memcpy_align_done
$memcpy_do_align:
	mov		ecx, 8			; a trick that's faster than rep movsb...
	sub		ecx, edi		; align destination to qword
	and		ecx, 111b		; get the low bits
	sub		ebx, ecx		; update copy count
	neg		ecx				; set up to jump into the array
	add		ecx, offset $memcpy_align_done
	jmp		ecx				; jump to array of movsb's

align 4
	movsb
	movsb
	movsb
	movsb
	movsb
	movsb
	movsb
	movsb

$memcpy_align_done:			; destination is dword aligned
	mov		ecx, ebx		; number of bytes left to copy
	shr		ecx, 6			; get 64-byte block count
	jz		$memcpy_ic_2	; finish the last few bytes

	cmp		ecx, IN_CACHE_COPY/64	; too big 4 cache? use uncached copy
	jae		$memcpy_uc_test

// This is small block copy that uses the MMX registers to copy 8 bytes
// at a time.  It uses the "unrolled loop" optimization, and also uses
// the software prefetch instruction to get the data into the cache.
align 16
$memcpy_ic_1:			; 64-byte block copies, in-cache copy

	prefetchnta [esi + (200*64/34+192)]		; start reading ahead

	movq	mm0, [esi+0]	; read 64 bits
	movq	mm1, [esi+8]
	movq	[edi+0], mm0	; write 64 bits
	movq	[edi+8], mm1	;    note:  the normal movq writes the
	movq	mm2, [esi+16]	;    data to cache; a cache line will be
	movq	mm3, [esi+24]	;    allocated as needed, to store the data
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

	add		esi, 64			; update source pointer
	add		edi, 64			; update destination pointer
	dec		ecx				; count down
	jnz		$memcpy_ic_1	; last 64-byte block?

$memcpy_ic_2:
	mov		ecx, ebx		; has valid low 6 bits of the byte count
$memcpy_ic_3:
	shr		ecx, 2			; dword count
	and		ecx, 1111b		; only look at the "remainder" bits
	neg		ecx				; set up to jump into the array
	add		ecx, offset $memcpy_last_few
	jmp		ecx				; jump to array of movsd's

$memcpy_uc_test:
	cmp		ecx, UNCACHED_COPY/64	; big enough? use block prefetch copy
	jae		$memcpy_bp_1

$memcpy_64_test:
	or		ecx, ecx		; tail end of block prefetch will jump here
	jz		$memcpy_ic_2	; no more 64-byte blocks left

// For larger blocks, which will spill beyond the cache, it's faster to
// use the Streaming Store instruction MOVNTQ.   This write instruction
// bypasses the cache and writes straight to main memory.  This code also
// uses the software prefetch instruction to pre-read the data.
align 16
$memcpy_uc_1:				; 64-byte blocks, uncached copy

	prefetchnta [esi + (200*64/34+192)]		; start reading ahead

	movq	mm0,[esi+0]		; read 64 bits
	add		edi,64			; update destination pointer
	movq	mm1,[esi+8]
	add		esi,64			; update source pointer
	movq	mm2,[esi-48]
	movntq	[edi-64], mm0	; write 64 bits, bypassing the cache
	movq	mm0,[esi-40]	;    note: movntq also prevents the CPU
	movntq	[edi-56], mm1	;    from READING the destination address
	movq	mm1,[esi-32]	;    into the cache, only to be over-written
	movntq	[edi-48], mm2	;    so that also helps performance
	movq	mm2,[esi-24]
	movntq	[edi-40], mm0
	movq	mm0,[esi-16]
	movntq	[edi-32], mm1
	movq	mm1,[esi-8]
	movntq	[edi-24], mm2
	movntq	[edi-16], mm0
	dec		ecx
	movntq	[edi-8], mm1
	jnz		$memcpy_uc_1	; last 64-byte block?

	jmp		$memcpy_ic_2		; almost done

// For the largest size blocks, a special technique called Block Prefetch
// can be used to accelerate the read operations.   Block Prefetch reads
// one address per cache line, for a series of cache lines, in a short loop.
// This is faster than using software prefetch, in this case.
// The technique is great for getting maximum read bandwidth,
// especially in DDR memory systems.
$memcpy_bp_1:			; large blocks, block prefetch copy

	cmp		ecx, CACHEBLOCK			; big enough to run another prefetch loop?
	jl		$memcpy_64_test			; no, back to regular uncached copy

	mov		eax, CACHEBLOCK / 2		; block prefetch loop, unrolled 2X
	add		esi, CACHEBLOCK * 64	; move to the top of the block
align 16
$memcpy_bp_2:
	mov		edx, [esi-64]		; grab one address per cache line
	mov		edx, [esi-128]		; grab one address per cache line
	sub		esi, 128			; go reverse order
	dec		eax					; count down the cache lines
	jnz		$memcpy_bp_2		; keep grabbing more lines into cache

	mov		eax, CACHEBLOCK		; now that it's in cache, do the copy
align 16
$memcpy_bp_3:
	movq	mm0, [esi   ]		; read 64 bits
	movq	mm1, [esi+ 8]
	movq	mm2, [esi+16]
	movq	mm3, [esi+24]
	movq	mm4, [esi+32]
	movq	mm5, [esi+40]
	movq	mm6, [esi+48]
	movq	mm7, [esi+56]
	add		esi, 64				; update source pointer
	movntq	[edi   ], mm0		; write 64 bits, bypassing cache
	movntq	[edi+ 8], mm1		;    note: movntq also prevents the CPU
	movntq	[edi+16], mm2		;    from READING the destination address 
	movntq	[edi+24], mm3		;    into the cache, only to be over-written,
	movntq	[edi+32], mm4		;    so that also helps performance
	movntq	[edi+40], mm5
	movntq	[edi+48], mm6
	movntq	[edi+56], mm7
	add		edi, 64				; update dest pointer

	dec		eax					; count down

	jnz		$memcpy_bp_3		; keep copying
	sub		ecx, CACHEBLOCK		; update the 64-byte block count
	jmp		$memcpy_bp_1		; keep processing chunks

// The smallest copy uses the X86 "movsd" instruction, in an optimized
// form which is an "unrolled loop".   Then it handles the last few bytes.
align 4
	movsd
	movsd			; perform last 1-15 dword copies
	movsd
	movsd
	movsd
	movsd
	movsd
	movsd
	movsd
	movsd			; perform last 1-7 dword copies
	movsd
	movsd
	movsd
	movsd
	movsd
	movsd

$memcpy_last_few:		; dword aligned from before movsd's
	mov		ecx, ebx	; has valid low 2 bits of the byte count
	and		ecx, 11b	; the last few cows must come home
	jz		$memcpy_final	; no more, let's leave
	rep		movsb		; the last 1, 2, or 3 bytes

$memcpy_final: 
	emms				; clean up the MMX state
	sfence				; flush the write buffer
	mov		eax, [dest]	; ret value = destination pointer

    }
}










	// align to 8 bytes
	// this align may be slower in [32kb, 64kb]
	// rationale: always called - it may speed up TINY as well
	// => we have to be careful to only copy min(8, 8-edi, size)
#define MEMCPY_ALIGN




// temporal (in-cache) copy; 64 bytes per iter
// This is small block copy that uses the MMX registers to copy 8 bytes
// at a time.  It uses the "unrolled loop" optimization, and also uses
// the software prefetch instruction to get the data into the cache.
#define MEMCPY_MOVQ




// 64 bytes per iter; uncached non-temporal
// For larger blocks, which will spill beyond the cache, it's faster to
// use the Streaming Store instruction MOVNTQ.   This write instruction
// bypasses the cache and writes straight to main memory.  This code also
// uses the software prefetch instruction to pre-read the data.
#define MEMCPY_MOVNTQ





// For the largest size blocks, a special technique called Block Prefetch
// can be used to accelerate the read operations.   Block Prefetch reads
// one address per cache line, for a series of cache lines, in a short loop.
// This is faster than using software prefetch, in this case.
// The technique is great for getting maximum read bandwidth,
// especially in DDR memory systems.
#define MEMCPY_BP_MOVNTQ








/*



void amd_memcpy_skeleton(u8 *dest, const u8 *src, size_t n)
{
  __asm {

	mov		ecx, [n]		; number of bytes to copy
	mov		edi, [dest]		; destination
	mov		esi, [src]		; source
	mov		ebx, ecx		; keep a copy of count

	cmp		ecx, TINY_BLOCK_COPY
	jb		$memcpy_ic_3	; tiny? skip mmx copy

	ALIGN

	mov		ecx, ebx		; number of bytes left to copy
	shr		ecx, 6			; get 64-byte block count
	jz		$memcpy_ic_2	; finish the last few bytes

	cmp		ecx, IN_CACHE_COPY/64	; too big 4 cache? use uncached copy
	jae		$memcpy_uc_test

	MMX_MOVQ

$memcpy_ic_2:
	mov		ecx, ebx		; has valid low 6 bits of the byte count
$memcpy_ic_3:
	TINY(<=64)

	emms				; clean up the MMX state
	sfence				; flush the write buffer
	ret

$memcpy_uc_test:
	cmp		ecx, UNCACHED_COPY/64	; big enough? use block prefetch copy
	jae		$memcpy_bp_1

$memcpy_64_test:
	or		ecx, ecx		; tail end of block prefetch will jump here
	jz		$memcpy_ic_2	; no more 64-byte blocks left

align 16
$memcpy_uc_1:				; 64-byte blocks, uncached copy
	PREFETCH_MOVNTQ

	jmp		$memcpy_ic_2		; almost done

$memcpy_bp_1:			; large blocks, block prefetch copy
	BLOCKPREFETCH_MOVNTQ
	// jumps to $memcpy_64_test when done
    }
}

									total taken jmp (no ret, no tbl)+unconditional
	0..64		tiny							0
	64..64k		mmx								1+1
	64k..192k	movntq							1+1
	192k..inf	blockprefetch					1+1


*/



static i64 t0, t1;
#define BEGIN\
	__asm pushad\
	__asm cpuid\
	__asm rdtsc\
	__asm mov dword ptr t0, eax\
	__asm mov dword ptr t0+4, edx\
	__asm popad
#define END\
	__asm pushad\
	__asm cpuid\
	__asm rdtsc\
	__asm mov dword ptr t1, eax\
	__asm mov dword ptr t1+4, edx\
	__asm popad
static const size_t MOVNTQ_MIN_THRESHOLD_64 = 64*KiB / 64;// upper limit for movq/movq copy w/SW prefetch

static const size_t BP_MIN_THRESHOLD_64 = 192*KiB / 64; // upper limit for movq/movntq w/SW prefetch

// rationale:
// - prefer "jnz loop" style vs "jz end; jmp loop" to make it
//   better for the branch prediction unit.
// - need declspec naked because we ret in the middle of the function (avoids jmp)
//   disadvantage: compiler cannot optimize param passing

void __declspec(naked) ia32_memcpy(void* dst, const void* src, size_t nbytes)
{
__asm {
	BEGIN
	mov		ecx, [esp+4+8]		;// nbytes
	mov		esi, [esp+4+4]		;// src
	mov		edi, [esp+4+0]		;// dst

mov		eax, 8
sub		eax, edi
and		eax, 0x07
cmp		eax, ecx
cmova	eax, ecx
sub		ecx, eax
neg		eax
add		eax, offset $align_table_end
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
$align_table_end:

	mov		ebx, ecx
	shr		ecx, 6			; # blocks

tiny:
	and		ebx, 63
	mov		edx, ebx
	shr		edx, 2			; dword count
	neg		edx
	add		edx, offset $movsd_table_end

	mov		eax, _bp
	cmp		ecx, BP_MIN_THRESHOLD_64
	cmovb	eax, _movntq
	cmp		ecx, MOVNTQ_MIN_THRESHOLD_64
	cmovb	eax, _mmx
	cmp		ecx, 64
	cmovbe	eax, edx
	jmp		eax

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
$movsd_table_end:

// TODO: move this calc into register
	mov		eax, ebx
	and		eax, 11b
	neg		eax
	add		eax, offset $movsb_table_end
	jmp		eax

	movsb
	movsb
	movsb
$movsb_table_end:
	END
	ret

_mmx:
	// we have >= 64
	align 16
$movq_loop:
prefetchnta [esi + (200*64/34+192)]
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
add		esi, 64
add		edi, 64
dec		ecx
jnz		$movq_loop
	emms
	jmp		tiny

_bp:
	// we have >= 8kb// until no more 8kb blocks
	$bp_process_chunk:
mov		eax, CACHEBLOCK / 2		; block prefetch loop, unrolled 2X
add		esi, CACHEBLOCK * 64	; move to the top of the block
align 16
/* touch each cache line in reverse order (prevents HW prefetch) */
$bp_prefetch_chunk:
mov		edx, [esi-64]
mov		edx, [esi-128]
sub		esi, 128
dec		eax
jnz		$bp_prefetch_chunk
mov		eax, CACHEBLOCK		/*; now that it's in cache, do the copy*/
align 16
$bp_copy_block:
movq	mm0, [esi+ 0]
movq	mm1, [esi+ 8]
movq	mm2, [esi+16]
movq	mm3, [esi+24]
movq	mm4, [esi+32]
movq	mm5, [esi+40]
movq	mm6, [esi+48]
movq	mm7, [esi+56]
add		esi, 64
movntq	[edi+ 0], mm0
movntq	[edi+ 8], mm1
movntq	[edi+16], mm2
movntq	[edi+24], mm3
movntq	[edi+32], mm4
movntq	[edi+40], mm5
movntq	[edi+48], mm6
movntq	[edi+56], mm7
add		edi, 64
dec		eax
jnz		$bp_copy_block
sub		ecx, CACHEBLOCK		/*; update the 64-byte block count*/
cmp		ecx, CACHEBLOCK
jl		$bp_process_chunk
	sfence
	emms
	// protect routine below
	cmp		ecx, 0
	jz		tiny

_movntq:
	// we have >= 64, 64B BLOCKS
	align 16
$movntq_loop:
prefetchnta [esi + (200*64/34+192)]
movq	mm0,[esi+0]
add		edi,64
movq	mm1,[esi+8]
add		esi,64
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
dec		ecx
movntq	[edi-8], mm1
jnz		$movntq_loop
	sfence
	emms
	jmp		tiny
}	// __asm
}	// ia32_memcpy



static void dtable_brep(u8* dst, const u8* src, size_t nbytes)
{
__asm
{
	BEGIN

	mov		esi, [src]
	mov		edi, [dst]
	mov		ecx, [nbytes]
	mov		ebx, ecx

	shr		ecx, 2			; dword count
	neg		ecx
	add		ecx, offset $movsd_table_end
	jmp		ecx				; jump to array of movsd's


	// The smallest copy uses the X86 "movsd" instruction, in an optimized
	// form which is an "unrolled loop".   Then it handles the last few bytes.
	align 4
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
$movsd_table_end:

	mov		ecx, ebx	; has valid low 2 bits of the byte count
	and		ecx, 11b	; the last few cows must come home
	jz		$memcpy_final	; no more, let's leave
	rep		movsb		; the last 1, 2, or 3 bytes
$memcpy_final:
	END
}
}



static void dtable_btable(u8* dst, const u8* src, size_t nbytes)
{
__asm
{
	BEGIN

	mov		esi, [src]
	mov		edi, [dst]
	mov		ecx, [nbytes]
	mov		ebx, ecx

	shr		ecx, 2			; dword count
	neg		ecx
	add		ecx, offset $movsd_table_end
	jmp		ecx				; jump to array of movsd's


	// The smallest copy uses the X86 "movsd" instruction, in an optimized
	// form which is an "unrolled loop".   Then it handles the last few bytes.
	align 4
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
$movsd_table_end:

	mov		ecx, ebx	; has valid low 2 bits of the byte count
	and		ecx, 11b	; the last few cows must come home
	neg		ecx
	add		ecx, offset $movsb_table_end
	jmp		ecx				; jump to array of movsb's

	movsb
	movsb
	movsb
$movsb_table_end:
	END
}
}


static void drep_brep(u8* dst, const u8* src, size_t nbytes)
{
__asm
{
	BEGIN

	mov		esi, [src]
	mov		edi, [dst]
	mov		ecx, [nbytes]
	mov		edx, ecx

	shr		ecx, 2			; dword count
rep	movsd

	mov		ecx, edx	; has valid low 2 bits of the byte count
	and		ecx, 11b	; the last few cows must come home
rep movsb
	END
}
}


static void brep(u8* dst, const u8* src, size_t nbytes)
{
__asm
{
	BEGIN

	mov		esi, [src]
	mov		edi, [dst]
	mov		ecx, [nbytes]
	mov		edx, ecx

rep	movsb
	END
}
}



static void bloop(u8* dst, const u8* src, size_t nbytes)
{
	BEGIN
	for(size_t i = 0; i < nbytes; i++)
		*dst++ = *src++;
	END
}

static void dloop_bloop(u8* dst, const u8* src, size_t nbytes)
{
	BEGIN
	size_t dwords = nbytes/4;
	for(size_t i = 0; i < dwords; i++)
	{
		*(u32*)dst = *(u32*)src;
		dst += 4; src += 4;
	}
	for(int i = 0; i < nbytes%4; i++)
		*dst++ = *(u8*)src++;
	END
}

static void _memcpy(u8* dst, const u8* src, size_t nbytes)
{
	BEGIN
	memcpy(dst, src, nbytes);
	END
}




static u8* setup_tiny_buf(int alignment, int misalign, bool is_dst)
{
	u8* p = (u8*)_aligned_malloc(256, alignment);
	p += misalign;
	if(is_dst)
	{
		for(int i = 0; i < 64; i++)
			p[i] = 0;
		for(int i = 0; i < 64; i++)
			p[i+64] = 0x80|i;
	}
	else
	{
		for(int i = 0; i < 64; i++)
			p[i] = i;
		for(int i = 0; i < 64; i++)
			p[i+64] = 0;
	}
	return p;
}

static void verify_tiny_buf(u8* p, size_t l, const char* culprit)
{
	for(size_t i = 0; i < l; i++)
		if(p[i] != i)
			debug_assert(0);

	for(int i = 0; i < 64; i++)
		if(p[i+64] != (0x80|i))
			debug_assert(0);
}


#define TIME(name, d, s, l)\
	least = 1000000;\
	for(int i = 0; i < 1000; i++)\
	{\
		name(d,s,l);\
		int clocks=(int)(t1-t0);\
		if(clocks < least) least=clocks;\
	}\
	verify_tiny_buf(d, l, #name);


static int brep_total, dtable_brep_total, dtable_btable_total, drep_brep_total;
static int bloop_total, dloop_bloop_total, _memcpy_total;

static void timeall(u8* d, const u8* s, size_t l)
{
	int least;

TIME(brep, d, s, l); brep_total += least;
TIME(dtable_brep, d, s, l); dtable_brep_total += least;
TIME(dtable_btable, d, s, l); dtable_btable_total += least;
TIME(drep_brep, d, s, l); drep_brep_total += least;

TIME(bloop, d, s, l); bloop_total += least;
TIME(dloop_bloop, d, s, l); dloop_bloop_total += least;
TIME(_memcpy, d, s, l); _memcpy_total += least;
}

/*
misalign 0
	brep: 8436
	dtable_brep: 7120
	dtable_btable: 6670
	drep_brep: 7384
8
	brep: 8436
	dtable_brep: 7120
	dtable_btable: 6670
	drep_brep: 7384

in release mode over all misaligns 0..63 and all sizes 1..64
                   total  difference WRT baseline (%)

dtable_btable:     434176
dtable_brep:       464001  6.9
memcpy:            499936 15.1
drep_brep:         502368 15.7
brep:              546752 26
dloop_bloop:       679426 56.5
bloop:             1537061
				   
in debug mode over all misaligns 0..63 and all sizes 1..64
	dtable_btable: 425728 0
	dtable_brep:   457153 7.3
	drep_brep:     494368 16.1
	brep:          538729 26.5

p3, debug mode over all misaligns 0..63 and all sizes 1..64
	dtable_btable: 1099605
	dtable_brep:   1111757	1.1
	memcpy:        1124093	2.2
	drep_brep:     1138089
	brep:          1313728
	dloop_bloop:   1756000
	bloop:         2405315

p3, release mode over all misaligns 0..63 and all sizes 1..64
	dtable_btable: 1092784
	dtable_brep:   1109136	1.5
	memcpy:        1116306	2.2
	drep_brep:     1127606
	dloop_bloop:   1129105
	brep:          1308052
	bloop:         1588062


*/

static void test_with_misalign(int misalign)
{
	u8* d = setup_tiny_buf(64, misalign, true);
	u8* s = setup_tiny_buf(64, misalign, false);

	for(int i = 0; i < 64; i++)
		timeall(d, s, i);
}

static void test_tiny()
{
	for(int i = 0; i < 64; i++)
		test_with_misalign(i);

	debug_printf("brep: %d\n", brep_total);
	debug_printf("dtable_brep: %d\n", dtable_brep_total);
	debug_printf("dtable_btable: %d\n", dtable_btable_total);
	debug_printf("drep_brep: %d\n", drep_brep_total);

	debug_printf("bloop: %d\n", bloop_total);
	debug_printf("dloop_bloop: %d\n", dloop_bloop_total);
	debug_printf("drep_memcpy_brep: %d\n", _memcpy_total);
}

static int test()
{
	test_tiny();
	return 0;
}
//int dummy = test();



















































//-----------------------------------------------------------------------------
// support code for lock-free primitives
//-----------------------------------------------------------------------------

// CAS does a sanity check on the location parameter to see if the caller
// actually is passing an address (instead of a value, e.g. 1). this is
// important because the call is via a macro that coerces parameters.
//
// reporting is done with the regular CRT assert instead of debug_assert
// because the wdbg code relies on CAS internally (e.g. to avoid
// nested stack traces). a bug such as VC's incorrect handling of params
// in __declspec(naked) functions would then cause infinite recursion,
// which is difficult to debug (since wdbg is hosed) and quite fatal.
#define ASSERT(x) assert(x)

// note: a 486 or later processor is required since we use CMPXCHG.
// there's no feature flag we can check, and the ia32 code doesn't
// bother detecting anything < Pentium, so this'll crash and burn if
// run on 386. we could replace cmpxchg with a simple mov (since 386
// CPUs aren't MP-capable), but it's not worth the trouble.

// note: don't use __declspec(naked) because we need to access one parameter
// from C code and VC can't handle that correctly.
bool __cdecl CAS_(uintptr_t* location, uintptr_t expected, uintptr_t new_value)
{
	// try to see if caller isn't passing in an address
	// (CAS's arguments are silently casted)
	ASSERT(!debug_is_pointer_bogus(location));

	bool was_updated;
__asm
{
	cmp		byte ptr [cpus], 1
	mov		eax, [expected]
	mov		edx, [location]
	mov		ecx, [new_value]
	je		$no_lock
_emit 0xf0	// LOCK prefix
$no_lock:
	cmpxchg	[edx], ecx
	sete	al
	mov		[was_updated], al
}
	return was_updated;
}


void atomic_add(intptr_t* location, intptr_t increment)
{
__asm
{
	cmp		byte ptr [cpus], 1
	mov		edx, [location]
	mov		eax, [increment]
	je		$no_lock
_emit 0xf0	// LOCK prefix
$no_lock:
	add		[edx], eax
}
}


// enforce strong memory ordering.
void mfence()
{
	// Pentium IV
	if(ia32_cap(SSE2))
		__asm mfence
}


void serialize()
{
	__asm cpuid
}

#else // i.e. #if !HAVE_ASM

bool CAS_(uintptr_t* location, uintptr_t expected, uintptr_t new_value)
{
	uintptr_t prev;

	debug_assert(location >= (uintptr_t*)0x10000);

	__asm__ __volatile__("lock; cmpxchgl %1,%2"
				 : "=a"(prev) // %0: Result in eax should be stored in prev
				 : "q"(new_value), // %1: new_value -> e[abcd]x
				   "m"(*location), // %2: Memory operand
				   "0"(expected) // Stored in same place as %0
				 : "memory"); // We make changes in memory
	return prev == expected;
}

void atomic_add(intptr_t* location, intptr_t increment)
{
	__asm__ __volatile__ (
			"cmpb $1, %1;"
			"je 1f;"
			"lock;"
			"1: addl %3, %0"
		: "=m" (*location) /* %0: Output into *location */
		: "m" (cpus), /* %1: Input for cpu check */
		  "m" (*location), /* %2: *location is also an input */
		  "r" (increment) /* %3: Increment (store in register) */
		: "memory"); /* clobbers memory (*location) */
}

void mfence()
{
	// no cpu caps stored in gcc compiles, so we can't check for SSE2 support
	/*
	if (ia32_cap(SSE2))
		__asm__ __volatile__ ("mfence");
	*/
}

void serialize()
{
	__asm__ __volatile__ ("cpuid");
}

#endif	// #if HAVE_ASM


//-----------------------------------------------------------------------------
// CPU / feature detect
//-----------------------------------------------------------------------------

//
// data returned by cpuid()
// each function using this data must call cpuid (no-op if already called)
//

static char vendor_str[13];
static int family, model, ext_family;
	// used in manual cpu_type detect
static u32 max_ext_func;

// caps
// treated as 128 bit field; order: std ecx, std edx, ext ecx, ext edx
// keep in sync with enum CpuCap and cpuid() code!
u32 caps[4];

static int have_brand_string = 0;
	// if false, need to detect cpu_type manually.
	// int instead of bool for easier setting from asm

// order in which registers are stored in regs array
enum Regs
{
	EAX,
	EBX,
	ECX,
	EDX
};

enum MiscCpuCapBits
{
	// AMD PowerNow! flags (returned in edx by CPUID 0x80000007)
	POWERNOW_FREQ_ID_CTRL = 2
};

static bool cpuid(u32 func, u32* regs)
{
	if(func > max_ext_func)
		return false;

	// (optimized for size)
__asm
{
	mov			eax, [func]
	cpuid
	mov			edi, [regs]
	stosd
	xchg		eax, ebx
	stosd
	xchg		eax, ecx
	stosd
	xchg		eax, edx
	stosd
}

	return true;
}


// (optimized for size)
static void cpuid()
{
__asm
{
	pushad										;// save ebx, esi, edi, ebp
		;// ICC7: pusha is the 16-bit form!

;// make sure CPUID is supported
	pushfd
	or			byte ptr [esp+2], 32
	popfd
	pushfd
	pop			eax
	shr			eax, 22							;// bit 21 toggled?
	jnc			no_cpuid

;// get vendor string
	xor			eax, eax
	cpuid
	mov			edi, offset vendor_str
	xchg		eax, ebx
	stosd
	xchg		eax, edx
	stosd
	xchg		eax, ecx
	stosd
	;// (already 0 terminated)

;// get CPU signature and std feature bits
	push		1
	pop			eax
	cpuid
	mov			[caps+0], ecx
	mov			[caps+4], edx
	movzx		edx, al
	shr			edx, 4
	mov			[model], edx					;// eax[7:4]
	movzx		edx, ah
	and			edx, 0x0f
	mov			[family], edx					;// eax[11:8]
	shr			eax, 20
	and			eax, 0x0f
	mov			[ext_family], eax				;// eax[23:20]

;// make sure CPUID ext functions are supported
	mov			esi, 0x80000000
	mov			eax, esi
	cpuid
	mov			[max_ext_func], eax
	cmp			eax, esi						;// max ext <= 0x80000000?
	jbe			no_ext_funcs					;// yes - no ext funcs at all
	lea			esi, [esi+4]					;// esi = 0x80000004
	cmp			eax, esi						;// max ext < 0x80000004?
	jb			no_brand_str					;// yes - brand string not available, skip

;// get CPU brand string (>= Athlon XP, P4)
	mov			edi, offset cpu_type
	push		-2
	pop			esi								;// loop counter: [-2, 0]
$1:	lea			eax, [0x80000004+esi]			;// 0x80000002 .. 4
	cpuid
	stosd
	xchg		eax, ebx
	stosd
	xchg		eax, ecx
	stosd
	xchg		eax, edx
	stosd
	inc			esi
	jle			$1
	;// (already 0 terminated)

	mov			[have_brand_string], esi		;// esi = 1 = true

no_brand_str:

;// get extended feature flags
	mov			eax, [0x80000001]
	cpuid
	mov			[caps+8], ecx
	mov			[caps+12], edx

no_ext_funcs:

no_cpuid:

	popad
}	// __asm
}	// cpuid()


bool ia32_cap(CpuCap cap)
{
	u32 idx = cap >> 5;
	if(idx > 3)
	{
		debug_warn("cap invalid");
		return false;
	}
	u32 bit = BIT(cap & 0x1f);

	return (caps[idx] & bit) != 0;
}



enum Vendor { UNKNOWN, INTEL, AMD };
static Vendor vendor = UNKNOWN;






static void get_cpu_type()
{
	// note: cpu_type is guaranteed to hold 48+1 chars, since that's the
	// length of the CPU brand string. strcpy(cpu_type, literal) is safe.

	// fall back to manual detect of CPU type if it didn't supply
	// a brand string, or if the brand string is useless (i.e. "Unknown").
	if(!have_brand_string || strncmp(cpu_type, "Unknow", 6) == 0)
		// we use an extra flag to detect if we got the brand string:
		// safer than comparing against the default name, which may change.
		//
		// some older boards reprogram the brand string with
		// "Unknow[n] CPU Type" on CPUs the BIOS doesn't recognize.
		// in that case, we ignore the brand string and detect manually.
	{
		if(vendor == AMD)
		{
			// everything else is either too old, or should have a brand string.
			if(family == 6)
			{
				if(model == 3 || model == 7)
					strcpy(cpu_type, "AMD Duron");	// safe
				else if(model <= 5)
					strcpy(cpu_type, "AMD Athlon");	// safe
				else
				{
					if(ia32_cap(MP))
						strcpy(cpu_type, "AMD Athlon MP");	// safe
					else
						strcpy(cpu_type, "AMD Athlon XP");	// safe
				}
			}
		}
		else if(vendor == INTEL)
		{
			// everything else is either too old, or should have a brand string.
			if(family == 6)
			{
				if(model == 1)
					strcpy(cpu_type, "Intel Pentium Pro");	// safe
				else if(model == 3 || model == 5)
					strcpy(cpu_type, "Intel Pentium II");	// safe
				else if(model == 6)
					strcpy(cpu_type, "Intel Celeron");		// safe
				else
					strcpy(cpu_type, "Intel Pentium III");	// safe
			}
		}
	}
	// we have a valid brand string; try to pretty it up some
	else
	{
		// strip (tm) from Athlon string
		if(!strncmp(cpu_type, "AMD Athlon(tm)", 14))
			memmove(cpu_type+10, cpu_type+14, 34);

		// remove 2x (R) and CPU freq from P4 string
		float freq;
		// the indicated frequency isn't necessarily correct - the CPU may be
		// overclocked. need to pass a variable though, since scanf returns
		// the number of fields actually stored.
		if(sscanf(cpu_type, " Intel(R) Pentium(R) 4 CPU %fGHz", &freq) == 1)
			strcpy(cpu_type, "Intel Pentium 4");	// safe
	}
}


static void measure_cpu_freq()
{
	// set max priority, to reduce interference while measuring.
	int old_policy; static sched_param old_param;	// (static => 0-init)
	pthread_getschedparam(pthread_self(), &old_policy, &old_param);
	static sched_param max_param;
	max_param.sched_priority = sched_get_priority_max(SCHED_FIFO);
	pthread_setschedparam(pthread_self(), SCHED_FIFO, &max_param);

	if(ia32_cap(TSC))
		// make sure the TSC is available, because we're going to
		// measure actual CPU clocks per known time interval.
		// counting loop iterations ("bogomips") is unreliable.
	{
		// note: no need to "warm up" cpuid - it will already have been
		// called several times by the time this code is reached.
		// (background: it's used in rdtsc() to serialize instruction flow;
		// the first call is documented to be slower on Intel CPUs)

		int num_samples = 16;
		// if clock is low-res, do less samples so it doesn't take too long.
		// balance measuring time (~ 10 ms) and accuracy (< 1 0/00 error -
		// ok for using the TSC as a time reference)
		if(timer_res() >= 1e-3)
			num_samples = 8;
		std::vector<double> samples(num_samples);

		int i;
		for(i = 0; i < num_samples; i++)
		{
			double dt;
			i64 dc;
				// i64 because VC6 can't convert u64 -> double,
				// and we don't need all 64 bits.

			// count # of clocks in max{1 tick, 1 ms}:
			// .. wait for start of tick.
			const double t0 = get_time();
			u64 c1; double t1;
			do
			{
				// note: get_time effectively has a long delay (up to 5 µs)
				// before returning the time. we call it before rdtsc to
				// minimize the delay between actually sampling time / TSC,
				// thus decreasing the chance for interference.
				// (if unavoidable background activity, e.g. interrupts,
				// delays the second reading, inaccuracy is introduced).
				t1 = get_time();
				c1 = rdtsc();
			}
			while(t1 == t0);
			// .. wait until start of next tick and at least 1 ms elapsed.
			do
			{
				const double t2 = get_time();
				const u64 c2 = rdtsc();
				dc = (i64)(c2 - c1);
				dt = t2 - t1;
			}
			while(dt < 1e-3);

			// .. freq = (delta_clocks) / (delta_seconds);
			//    cpuid/rdtsc/timer overhead is negligible.
			const double freq = dc / dt;
			samples[i] = freq;
		}

		std::sort(samples.begin(), samples.end());

		// median filter (remove upper and lower 25% and average the rest).
		// note: don't just take the lowest value! it could conceivably be
		// too low, if background processing delays reading c1 (see above).
		double sum = 0.0;
		const int lo = num_samples/4, hi = 3*num_samples/4;
		for(i = lo; i < hi; i++)
			sum += samples[i];
		cpu_freq = sum / (hi-lo);

	}
	// else: TSC not available, can't measure; cpu_freq remains unchanged.

	// restore previous policy and priority.
	pthread_setschedparam(pthread_self(), old_policy, &old_param);
}


int get_cur_processor_id()
{
	int apic_id;
	__asm {
	push		1
	pop			eax
	cpuid
	shr			ebx, 24
	mov			[apic_id], ebx					; ebx[31:24]
	}

	return apic_id;
}


// set cpu_smp if there's more than 1 physical CPU -
// need to know this for wtime's TSC safety check.
// called on each CPU by on_each_cpu.
static void check_smp()
{
	debug_assert(cpus > 0 && "must know # CPUs (call OS-specific detect first)");

	// we don't check if it's Intel and P4 or above - HT may be supported
	// on other CPUs in future. haven't come across a processor that
	// incorrectly sets the HT feature bit.
	if(!ia32_cap(HT))
	{
		// no HT supported, just check number of CPUs as reported by OS.
		cpu_smp = (cpus > 1);
		return;
	}

	// first call. we set cpu_smp below if more than 1 physical CPU is found,
	// so clear it until then.
	if(cpu_smp == -1)
		cpu_smp = 0;


	//
	// still need to check if HT is actually enabled (BIOS and OS);
	// there might be 2 CPUs with HT supported but disabled.
	//

	// get number of logical CPUs per package
	// (the same for all packages on this system)
	int log_cpus_per_package;
	__asm {
	push		1
	pop			eax
	cpuid
	shr			ebx, 16
	and			ebx, 0xff
	mov			log_cpus_per_package, ebx		; ebx[23:16]
	}

	// logical CPUs are initialized after one another =>
	// they have the same physical ID.
	const int id = get_cur_processor_id();
	const int phys_shift = log2(log_cpus_per_package);
	const int phys_id = id >> phys_shift;

	// more than 1 physical CPU found
	static int last_phys_id = -1;
	if(last_phys_id != -1 && last_phys_id != phys_id)
		cpu_smp = 1;
	last_phys_id = phys_id;
}


static void check_speedstep()
{
	if(vendor == INTEL)
	{
		if(ia32_cap(EST))
			cpu_speedstep = 1;
	}
	else if(vendor == AMD)
	{
		u32 regs[4];
		if(cpuid(0x80000007, regs))
			if(regs[EDX] & POWERNOW_FREQ_ID_CTRL)
				cpu_speedstep = 1;
	}
}


void ia32_get_cpu_info()
{
	cpuid();
	if(family == 0)	// cpuid not supported - can't do the rest
		return;

	// (for easier comparison)
	if(!strcmp(vendor_str, "AuthenticAMD"))
		vendor = AMD;
	else if(!strcmp(vendor_str, "GenuineIntel"))
		vendor = INTEL;

	get_cpu_type();
	check_speedstep();
	on_each_cpu(check_smp);

	measure_cpu_freq();

	// HACK: on Windows, the HRT makes its final implementation choice
	// in the first calibrate call where cpu info is available.
	// call wtime_reset_impl here to have that happen now,
	// so app code isn't surprised by a timer change, although the HRT
	// does try to keep the timer continuous.
#if OS_WIN
	wtime_reset_impl();
#endif
}
