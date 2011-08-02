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

#include "lib/self_test.h"

#include "maths/Random.h"

class TestRandom : public CxxTest::TestSuite
{
public:
	void test_sequence()
	{
		uint32_t seed[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
		WELL512 rng;
		rng.seed(seed);

		// Result vector was generated via http://www.iro.umontreal.ca/~panneton/well/WELL512a.c

		uint32_t expected[] = { 2423521338U, 2295858494U, 4237567038U, 3767796794U, 3498837026U, 3236431142U, 3162776622U,
				2695103786U, 2495358234U, 3292178890U, 1194984719U, 1635922038U, 1902646777U, 3540127820U, 2776150123U,
				3803026854U, 4022481096U, 2890771780U, 2383096164U, 2598906205U, 641858878U, 703208507U, 3187021161U, 4164927930U,
				1729358216U, 3970992993U, 55500867U, 1064458966U, 3142916837U, 2792079210U, 415718876U, 2069253743U, 1090459178U,
				1150648555U, 2130138587U, 85555256U, 938493562U, 573358422U, 2500660050U, 3673342299U, 4062731563U, 4281407583U,
				131831842U, 2943104616U, 260524413U, 4185358860U, 1146840142U, 173433925U, 962932907U, 3625667774U, 2966176140U,
				623826899U, 3222323490U, 1781272619U, 2042445736U, 145932495U, 2676651172U, 1245572857U, 2279822797U, 1079936135U,
				3912374806U, 3819369956U, 2524610988U, 2526918697U };

		for (size_t i = 0; i < ARRAY_SIZE(expected); ++i)
			TS_ASSERT_EQUALS(rng(), expected[i]);

		for (size_t i = 0; i < 1024*1024; ++i)
			rng();

		TS_ASSERT_EQUALS(rng(), 2143457027U);
	}

	void test_seed()
	{
		WELL512 rng;
		rng.seed(1234);

		// Equivalent to:
		// uint32_t seed[] = { 1234, 3159640283, 4062961311, 3954462607, 3112783424, 2849714703, 731821095, 2232873578, 1251953424,
		// 		3917199038, 231030171, 268845362, 2344188614, 1849447265, 2383103214, 2227731755 };

		// Result vector was generated via http://www.iro.umontreal.ca/~panneton/well/WELL512a.c

		uint32_t expected[] = { 3796049263U, 3121637295U, 2314947441U, 124483343U, 1062552979U, 2701629854U, 2863421760U,
				1833958548U, 3210932400U, 3192737225U, 3835676748U, 248447399U, 1120349102U, 2349012450U, 2210895795U, 564651791U,
				4272914221U, 3396120565U, 419954947U, 1558309715U, 2980408554U, 3700173484U, 843419536U, 1226049545U, 383882930U,
				4147762817U, 3046810299U, 2822903886U, 2009534132U, 3375709471U, 816292113U, 4153554971U, 1025098704U, 3084937380U,
				3413388140U, 1868518045U, 1137536168U, 2321169U, 1171973932U, 2527937887U, 2851724729U, 180120257U, 3972785449U,
				3106766194U, 4238600917U, 3828143376U, 3332379518U, 3410880226U, 1358510220U, 1680847693U, 3264109669U,
				4214219167U, 635098860U, 3999351741U, 4274606842U, 2037105003U, 1521922362U, 2677146791U, 3449144249U, 3347816516U,
				44838202U, 3818140137U, 1521668346U, 260162572U };

		for (size_t i = 0; i < ARRAY_SIZE(expected); ++i)
			TS_ASSERT_EQUALS(rng(), expected[i]);
	}

	void test_comparable()
	{
		WELL512 rng1;
		WELL512 rng2;
		TS_ASSERT(rng1 == rng2);

		rng1.seed(1234);
		TS_ASSERT(!(rng1 == rng2));

		rng2.seed(1234);
		TS_ASSERT(rng1 == rng2);

		rng1();
		TS_ASSERT(!(rng1 == rng2));

		rng2 = rng1;
		TS_ASSERT(rng1 == rng2);

		WELL512 rng3(rng2);
		TS_ASSERT(rng2 == rng3);
	}

	void test_stream()
	{
		WELL512 rng1;
		WELL512 rng2;
		rng1();

		std::stringstream str;
		str << rng1;

		TS_ASSERT_STR_EQUALS(str.str(),
				"15 2666258328 1 1812433255 1900727105 1208447044 2481403966 "
				"4042607538 337614300 3232553940 1018809052 3202401494 "
				"1775180719 3192392114 594215549 184016991 1235243591 ")

		TS_ASSERT(!(rng1 == rng2));
		str >> rng2;
		TS_ASSERT(rng1 == rng2);
	}
};
