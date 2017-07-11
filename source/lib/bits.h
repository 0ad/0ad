/* Copyright (C) 2010 Wildfire Games.
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
inline T Bit(size_t n)
{
	const T one = T(1);
	return (T)(one << n);
}

/**
 * pretty much the same as Bit\<unsigned\>.
 * this is intended for the initialization of enum values, where a
 * compile-time constant is required.
 **/
#define BIT(n) (1u << (n))

template<typename T>
inline bool IsBitSet(T value, size_t index)
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
inline T bit_mask(size_t numBits)
{
	const T bitsInT = sizeof(T)*CHAR_BIT;
	const T allBits = (T)~T(0);
	// (shifts of at least bitsInT are undefined)
	if(numBits >= bitsInT)
		return allBits;
	// (note: the previous allBits >> (bitsInT-numBits) is not safe
	// because right-shifts of negative numbers are undefined.)
	const T mask = (T)((T(1) << numBits)-1);
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
	const size_t numBits = (hi_idx - lo_idx)+1;	// # bits to return
	T result = T(num >> lo_idx);
	result = T(result & bit_mask<T>(numBits));
	return result;
}

/**
 * set the value of bits hi_idx:lo_idx
 *
 * @param lo_idx bit index of lowest  bit to include
 * @param hi_idx bit index of highest bit to include
 * @param value new value to be assigned to these bits
 **/
template<typename T>
inline T SetBitsTo(T num, size_t lo_idx, size_t hi_idx, size_t value)
{
	const size_t numBits = (hi_idx - lo_idx)+1;
	ASSERT(value < (T(1) << numBits));
	const T mask = bit_mask<T>(numBits) << lo_idx;
	T result = num & ~mask;
	result = T(result | (value << lo_idx));
	return result;
}


/**
 * @return number of 1-bits in mask.
 * execution time is proportional to number of 1-bits in mask.
 **/
template<typename T>
inline size_t SparsePopulationCount(T mask)
{
	size_t num1Bits = 0;
	while(mask)
	{
		mask &= mask-1; // clear least significant 1-bit
		num1Bits++;
	}

	return num1Bits;
}

/**
 * @return number of 1-bits in mask.
 * execution time is logarithmic in the total number of bits.
 * supports up to 128-bit integers (if their arithmetic operators are defined).
 * [http://graphics.stanford.edu/~seander/bithacks.html#CountBitsSetParallel]
 **/
template<typename T>
static inline size_t PopulationCount(T x)
{
	cassert(!std::numeric_limits<T>::is_signed);
	const T mask = T(~T(0));
	x -= (x >> 1) & (mask/3);	// count 2 bits
	x = (x & (mask/15*3)) + ((x >> 2) & (mask/15*3));	// count 4 bits
	x = (x + (x >> 4)) & (mask/255*15);	// count 8 bits
	return T(x * (mask/255)) >> ((sizeof(T)-1)*CHAR_BIT);
}



/**
 * @return whether the given number is a power of two.
 **/
template<typename T>
inline bool is_pow2(T n)
{
	// 0 would pass the test below but isn't a POT.
	if(n == 0)
		return false;
	return (n & (n-1)) == 0;
}

// as above; intended for use in static_assert
#define IS_POW2(n) (((n) != 0) && ((n) & ((n)-1)) == 0)

template<typename T>
inline T LeastSignificantBit(T x)
{
	const T negX = T(~x + 1);	// 2's complement (avoids 'negating unsigned type' warning)
	return x & negX;
}

template<typename T>
inline T ClearLeastSignificantBit(T x)
{
	return x & (x-1);
}


/**
 * ceil(log2(x))
 *
 * @param x (unsigned integer)
 * @return ceiling of the base-2 logarithm (i.e. rounded up) or
 * zero if the input is zero.
 **/
template<typename T>
inline size_t ceil_log2(T x)
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

// compile-time variant of the above
template<size_t N>
struct CeilLog2
{
	enum { value = 1 + CeilLog2<(N+1)/2>::value };
};

template<>
struct CeilLog2<1>
{
	enum { value = 0 };
};

template<>
struct CeilLog2<0>
{
	enum { value = 0 };
};



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
inline T round_up_to_pow2(T x)
{
	return T(1) << ceil_log2(x);
}

/**
 * round down to next larger power of two.
 **/
template<typename T>
inline T round_down_to_pow2(T x)
{
	return T(1) << floor_log2(x);
}

/**
 * round number up/down to the next given multiple.
 *
 * @param n Number to round.
 * @param multiple Must be a power of two.
 **/
template<typename T>
inline T round_up(T n, T multiple)
{
	ASSERT(is_pow2(multiple));
	const T result = (n + multiple-1) & ~(multiple-1);
	ASSERT(n <= result && result < n+multiple);
	return result;
}

template<typename T>
inline T round_down(T n, T multiple)
{
	ASSERT(is_pow2(multiple));
	const T result = n & ~(multiple-1);
	ASSERT(result <= n && n < result+multiple);
	return result;
}

// evaluates to an expression suitable as an initializer
// for constant static data members.
#define ROUND_UP(n, multiple) (((n) + (multiple)-1) & ~((multiple)-1))


template<typename T>
inline T MaxPowerOfTwoDivisor(T value)
{
	ASSERT(value != T(0));

	for(size_t log2 = 0; log2 < sizeof(T)*CHAR_BIT; log2++)
	{
		if(IsBitSet(value, log2))
			return T(1) << log2;
	}

	DEBUG_WARN_ERR(ERR::LOGIC);	// unreachable (!= 0 => there is a set bit)
	return 0;
}

#endif	// #ifndef INCLUDED_BITS
