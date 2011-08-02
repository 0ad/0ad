/* Copyright (C) 2011 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef INCLUDED_MATHS_RANDOM
#define INCLUDED_MATHS_RANDOM

/**
 * Random number generator with period 2^{512}-1;
 * effectively a better version of MT19937 (smaller state, similarly fast,
 * simpler code, better distribution).
 * Implements Boost.Random's PseudoRandomNumberGenerator concept.
 */
class WELL512
{
private:
	uint32_t state[16];
	uint32_t index;

public:
	WELL512()
	{
		seed((uint32_t)0);
	}

	uint32_t operator()()
	{
		// WELL512 implementation by Chris Lomont (Game Programming Gems 7;
		// http://lomont.org/Math/Papers/2008/Lomont_PRNG_2008.pdf)

		uint32_t a, b, c, d;
		a = state[index];
		c = state[(index + 13) & 15];
		b = a ^ c ^ (a << 16) ^ (c << 15);
		c = state[(index + 9) & 15];
		c ^= (c >> 11);
		a = state[index] = b ^ c;
		d = a ^ ((a << 5) & 0xDA442D24UL);
		index = (index + 15) & 15;
		a = state[index];
		state[index] = a ^ b ^ d ^ (a << 2) ^ (b << 18) ^ (c << 28);
		return state[index];
	}

	void seed(uint32_t value)
	{
		index = 0;

		// Expand the seed with the same algorithm as boost::random::mersenne_twister
		const uint32_t mask = ~0u;
		state[0] = value & mask;
		for (uint32_t i = 1; i < ARRAY_SIZE(state); ++i)
			state[i] = (1812433253UL * (state[i - 1] ^ (state[i - 1] >> 30)) + i) & mask;
	}

	void seed(uint32_t values[16])
	{
		index = 0;

		for (uint32_t i = 0; i < ARRAY_SIZE(state); ++i)
			state[i] = values[i];
	}

	// Implement UniformRandomNumberGenerator concept:

	typedef uint32_t result_type;

	uint32_t min() const
	{
		return std::numeric_limits<uint32_t>::min();
	}

	uint32_t max() const
	{
		return std::numeric_limits<uint32_t>::max();
	}

	// Implement EqualityComparable concept:

	friend bool operator==(const WELL512& x, const WELL512& y)
	{
		if (x.index != y.index)
			return false;
		for (uint32_t i = 0; i < ARRAY_SIZE(state); ++i)
			if (x.state[i] != y.state[i])
				return false;
		return true;
	}

	friend bool operator!=(const WELL512& x, const WELL512& y)
	{
		return !(x == y);
	}

	// Implement Streamable concept (based on boost::random::mersenne_twister):

	template<class CharT, class Traits>
	friend std::basic_ostream<CharT, Traits>&
	operator<<(std::basic_ostream<CharT, Traits>& os, const WELL512& rng)
	{
		os << rng.index << " ";
		for (uint32_t i = 0; i < ARRAY_SIZE(rng.state); ++i)
			os << rng.state[i] << " ";
		return os;
	}

	template<class CharT, class Traits>
	friend std::basic_istream<CharT, Traits>&
	operator>>(std::basic_istream<CharT, Traits>& is, WELL512& rng)
	{
		is >> rng.index >> std::ws;
		for (uint32_t i = 0; i < ARRAY_SIZE(rng.state); ++i)
			is >> rng.state[i] >> std::ws;
		return is;
	}
};

#endif // INCLUDED_MATHS_RANDOM
