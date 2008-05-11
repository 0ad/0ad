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
template<typename T>
T bit_mask(size_t num_bits)
{
	return (T)(T(1) << num_bits)-1;
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
template<typename T>
inline T bits(T num, size_t lo_idx, size_t hi_idx)
{
	const size_t count = (hi_idx - lo_idx)+1;	// # bits to return
	T result = num >> lo_idx;
	result &= bit_mask<T>(count);
	return result;
}

/**
 * @return number of 1-bits in mask
 **/
template<typename T>
size_t PopulationCount(T mask)
{
	// note: a more complex but probably faster method is given at
	// http://graphics.stanford.edu/~seander/bithacks.html#CountBitsSetParallel

	size_t num1Bits = 0;
	while(mask)
	{
		mask &= mask-1; // clear least significant 1-bit
		num1Bits++;
	}

	return num1Bits;
}

/**
 * @return whether the given number is a power of two.
 **/
template<typename T>
bool is_pow2(T n)
{
	// 0 would pass the test below but isn't a POT.
	if(n == 0)
		return false;
	return (n & (n-1)) == 0;
}

/**
 * ceil(log2(n))
 *
 * @param n (integer) input; MUST be > 0, else results are undefined.
 * @return ceiling of the base-2 logarithm (i.e. rounded up).
 **/
template<typename T>
size_t ceil_log2(T x)
{
	T bit = 1;
	size_t log = 0;
	while(bit < x && bit != 0)	// must detect overflow
	{
		log++;
		bit *= 2;
	}

	return log;
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
template<typename T>
T round_up_to_pow2(T x)
{
	return T(1) << ceil_log2(x);
}

/**
 * round number up/down to the next given multiple.
 *
 * @param multiple: must be a power of two.
 **/
template<typename T>
T round_up(T n, T multiple)
{
	debug_assert(is_pow2(multiple));
	const T result = (n + multiple-1) & ~(multiple-1);
	debug_assert(n <= result && result < n+multiple);
	return result;
}

template<typename T>
T round_down(T n, T multiple)
{
	debug_assert(is_pow2(multiple));
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
