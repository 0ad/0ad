/**
 * =========================================================================
 * File        : bits.cpp
 * Project     : 0 A.D.
 * Description : bit-twiddling.
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#include "precompiled.h"
#include "bits.h"

#if ARCH_IA32
# include "lib/sysdep/ia32/ia32_asm.h"	// ia32_asm_log2_of_pow2
#endif


int log2_of_pow2(uint n)
{
	int bit_index;

#if ARCH_IA32
	bit_index = ia32_asm_log2_of_pow2(n);
#else
	if(!is_pow2(n))
		bit_index = -1;
	else
	{
		bit_index = 0;
		// note: compare against n directly because it is known to be a POT.
		for(uint bit_value = 1; bit_value != n; bit_value *= 2)
			bit_index++;
	}
#endif

	debug_assert(-1 <= bit_index && bit_index < (int)sizeof(int)*CHAR_BIT);
	debug_assert(bit_index == -1 || n == (1u << bit_index));
	return bit_index;
}


int floor_log2(const float x)
{
	const u32 i = *(u32*)&x;
	u32 biased_exp = (i >> 23) & 0xFF;
	return (int)biased_exp - 127;
}


uint round_up_to_pow2(uint x)
{
	// fold upper bit into lower bits; leaves same MSB set but
	// everything below it 1. adding 1 yields next POT.
	x |= (x >> 1);
	x |= (x >> 2);
	x |= (x >> 4);
	x |= (x >> 8);
	x |= (x >> 16);
	// if ints are 64 bits, add "x |= (x >> 32);"
	cassert(sizeof(int)*CHAR_BIT == 32);

	return x+1;
}
