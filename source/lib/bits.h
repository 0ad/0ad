/* Copyright (c) 2010 Wildfire Games
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * bit-twiddling.
 */

#ifndef INCLUDED_BITS
#define INCLUDED_BITS

/**
 * value of bit number \<n\>.
 *
 * @param n bit index.
 *
 * requirements:
 * - T should be an unsigned type
 * - n must be in [0, CHAR_BIT*sizeof(T)), else the result is undefined!
 **/
template<typename T>
T Bit(size_t n)
{
	const T one = T(1);
	return (one << n);
}

/**
 * pretty much the same as Bit\<unsigned\>.
 * this is intended for the initialization of enum values, where a
 * compile-time constant is required.
 **/
#define BIT(n) (1u << (n))

template<typename T>
bool IsBitSet(T value, size_t index)
{
	const T bit = Bit<T>(index);
	return (value & bit) != 0;
}


// these are declared in the header and inlined to aid compiler optimizations
// (they can easily end up being time-critical).
// note: GCC can't inline extern functions, while VC's "Whole Program
// Optimization" can.

/**
 * a mask that includes the lowest N bits
 *
 * @param numBits Number of bits in mask.
 **/
template<typename T>
T bit_mask(size_t numBits)
{
	if(numBits == 0)	// prevent shift count == bitsInT, which would be undefined.
		return 0;
	// notes:
	// - the perhaps more intuitive (1 << numBits)-1 cannot
	//   handle numBits == bitsInT, but this implementation does.
	// - though bulky, the below statements avoid sign-conversion warnings.
	const T bitsInT = sizeof(T)*CHAR_BIT;
	T mask(0);
	mask = T(~mask);
	mask >>= T(bitsInT-numBits);
	return mask;
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
	T result = T(num >> lo_idx);
	result = T(result & bit_mask<T>(count));
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
 * ceil(log2(x))
 *
 * @param x (unsigned integer)
 * @return ceiling of the base-2 logarithm (i.e. rounded up) or
 * zero if the input is zero.
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
 * @param x (float) input; MUST be > 0, else results are undefined.
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
 * @param n Number to round.
 * @param multiple Must be a power of two.
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


template<typename T>
T MaxPowerOfTwoDivisor(T value)
{
	debug_assert(value != T(0));

	for(size_t log2 = 0; log2 < sizeof(T)*CHAR_BIT; log2++)
	{
		if(IsBitSet(value, log2))
			return T(1) << log2;
	}

	debug_assert(0);	// unreachable (!= 0 => there is a set bit)
	return 0;
}


#endif	// #ifndef INCLUDED_BITS
