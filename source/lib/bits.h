/**
 * =========================================================================
 * File        : bits.h
 * Project     : 0 A.D.
 * Description : bit-twiddling.
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#ifndef INCLUDED_BITS
#define INCLUDED_BITS

/**
 * value of bit number <n> as unsigned long.
 *
 * @param n bit index (0..CHAR_BIT*sizeof(int)-1)
 **/
#define BIT(n) (1ul << (n))

/**
 * value of bit number <n> as unsigned long long.
 *
 * @param n bit index (0..CHAR_BIT*sizeof(int)-1)
 **/
#define BIT64(n) (1ull << (n))


// these are declared in the header and inlined to aid compiler optimizations
// (they can easily end up being time-critical).
// note: GCC can't inline extern functions, while VC's "Whole Program
// Optimization" can.

/**
 * a mask that includes the lowest N bits
 *
 * @param num_bits number of bits in mask
 **/
inline uint bit_mask(uint num_bits)
{
	return (1u << num_bits)-1;
}

inline u64 bit_mask64(uint num_bits)
{
	return (1ull << num_bits)-1;
}

/**
 * extract the value of bits hi_idx:lo_idx within num
 *
 * example: bits(0x69, 2, 5) == 0x0A
 *
 * @param num number whose bits are to be extracted
 * @param lo_idx bit index of lowest  bit to include
 * @param hi_idx bit index of highest bit to include
 * @return value of extracted bits.
 **/
inline uint bits(uint num, uint lo_idx, uint hi_idx)
{
	const uint count = (hi_idx - lo_idx)+1;	// # bits to return
	uint result = num >> lo_idx;
	result &= bit_mask(count);
	return result;
}

inline u64 bits64(u64 num, uint lo_idx, uint hi_idx)
{
	const uint count = (hi_idx - lo_idx)+1;	// # bits to return
	u64  result = num >> lo_idx;
	result &= bit_mask64(count);
	return result;
}

/**
 * @return whether the given number is a power of two.
 **/
inline bool is_pow2(uint n)
{
	// 0 would pass the test below but isn't a POT.
	if(n == 0)
		return false;
	return (n & (n-1ul)) == 0;
}


/**
 * @return the (integral) base 2 logarithm, or -1 if the number
 * is not a power-of-two.
 **/
extern int log2_of_pow2(uint n);

/**
 * ceil(log2(n))
 *
 * @param n (integer) input; MUST be > 0, else results are undefined.
 * @return ceiling of the base-2 logarithm (i.e. rounded up).
 **/
inline uint ceil_log2(uint x)
{
	uint bit = 1;
	uint l = 0;
	while(bit < x && bit != 0)	// must detect overflow
	{
		l++;
		bit += bit;
	}

	return l;
}


/**
 * floor(log2(f))
 * fast, uses the FPU normalization hardware.
 *
 * @param f (float) input; MUST be > 0, else results are undefined.
 * @return floor of the base-2 logarithm (i.e. rounded down).
 **/
extern int floor_log2(const float x);

/**
 * round up to next larger power of two.
 **/
extern uint round_up_to_pow2(uint x);

/**
 * round number up/down to the next given multiple.
 *
 * @param multiple: must be a power of two.
 **/
template<typename T>
T round_up(T n, T multiple)
{
	debug_assert(is_pow2((uint)multiple));
	const T result = (n + multiple-1) & ~(multiple-1);
	debug_assert(n <= result && result < n+multiple);
	return result;
}

template<typename T>
T round_down(T n, T multiple)
{
	debug_assert(is_pow2((uint)multiple));
	const T result = n & ~(multiple-1);
	debug_assert(result <= n && n < result+multiple);
	return result;
}


template<typename T>
bool IsAligned(T t, uintptr_t multiple)
{
	return ((uintptr_t)t % multiple) == 0;
}

#endif	// #ifndef INCLUDED_BITS
