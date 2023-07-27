/* Copyright (C) 2021 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "lib/self_test.h"

#include "lib/timer.h"
#include "ps/Hashing.h"

class TestHashing : public CxxTest::TestSuite
{
public:
	void test_hash_cryptographically()
	{
		// Simple test: these should be deterministic and no collision on these trivial cases
		TS_ASSERT_EQUALS(HashCryptographically("", ""), "");
		TS_ASSERT_EQUALS(HashCryptographically("", "foo"), "");

		TS_ASSERT_EQUALS(HashCryptographically("pass", ""), "CFD946EEBCC23A1642BD846FF54B3659765D305D352D6C590DCCE0728BAAE360");
		TS_ASSERT_EQUALS(HashCryptographically("pass", "foo"), "7E18AD648A3BE29EC513551D54AFD2505F9726EE14750BCC92F979D41A3328D3");
		TS_ASSERT_EQUALS(HashCryptographically("pass", "foofoo"), "5CCF3FCD4A285A133F19461ADE1819C838E16D9C1BED3B18276B5F5EBF57E172");
		TS_ASSERT_EQUALS(HashCryptographically("pass", "bar"), "5195DE5588213B1A07BCA48BB43050EE2C1FD99DC37E243B313D3E12A6AAFFD8");
		TS_ASSERT_EQUALS(HashCryptographically("pass", "foobar"), "E63C16BBE04E806DC54032A0D4BAABB96A99E0DA695357035E23C83A4E20E718");
		TS_ASSERT_EQUALS(HashCryptographically("pass", ""), "CFD946EEBCC23A1642BD846FF54B3659765D305D352D6C590DCCE0728BAAE360");

		TS_ASSERT_EQUALS(HashCryptographically("passpass", ""), "68FCE509D0B68EC7142D28165E6D697E26FEB929FCC1FE70ED4D0A8C716F7E56");
		TS_ASSERT_EQUALS(HashCryptographically("passpass", "foo"), "B766D8DB7AD9D110ED6059BAC6E3667486609AE193FF62ADB4EE174AC665F6F8");
		TS_ASSERT_EQUALS(HashCryptographically("passpass", "foofoo"), "0BDD6EE3B37FB7B6B4AA24F24AD148CED9BC26793B5EDBF68598800F5F53FD77");
		TS_ASSERT_EQUALS(HashCryptographically("passpass", "bar"), "BB719CDF8E5E0505AFEEABC487BE4A2A2EE83683DEC6BFD5A08C2E6C308A51C2");
		TS_ASSERT_EQUALS(HashCryptographically("passpass", "foobar"), "6745DD30BAD7B7A78BC0DC559C684CD4A5E13AD538CDE23D75577B61943D3DC1");

		// Test that hashing hashes works.
		TS_ASSERT_EQUALS(HashCryptographically("A989A9C5BDB02DD91C038661424BE039E2AE727483A30D3F13F995D0AB6C3712", "foobar"), "9509646C4675EED47E9D49AF20456F3F08605E87CA825DD44A846F5E3E3AC02F");
		TS_ASSERT_EQUALS(HashCryptographically("D9895FDEE287DBEE19907B7329207F388B1708AC4A123CA537603E953885B20F", "foobar"), "8CE4D45113D5A682FE4B6F185C1880F83EEA6CB2F007E815DCA5BF4B8178ECD0");
	}

	void test_hash_perf_DISABLED()
	{
		double t = timer_Time();
		for (size_t i = 0; i < 100; ++i)
			HashCryptographically("somePasswordValue", reinterpret_cast<const char*>(&i));
		double total = timer_Time() - t;
		printf("Time: %lfs\n", total);
	}
};
