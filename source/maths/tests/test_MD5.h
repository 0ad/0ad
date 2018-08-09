/* Copyright (C) 2018 Wildfire Games.
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

#include "maths/MD5.h"
#include "ps/Util.h"

class TestMD5 : public CxxTest::TestSuite
{
public:
	std::string decode(u8* digest)
	{
		return Hexify(digest, MD5::DIGESTSIZE);
	}

	void compare(const char* input, const char* expected)
	{
		u8 digest[MD5::DIGESTSIZE];

		MD5 m;
		m.Update((const u8*)input, strlen(input));
		m.Final(digest);

		TSM_ASSERT_STR_EQUALS(input, decode(digest), expected);
	}

	void test_rfc()
	{
		compare("", "d41d8cd98f00b204e9800998ecf8427e");
		compare("a", "0cc175b9c0f1b6a831c399e269772661");
		compare("abc", "900150983cd24fb0d6963f7d28e17f72");
		compare("message digest", "f96b697d7cb7938d525a2f31aaf161d0");
		compare("abcdefghijklmnopqrstuvwxyz", "c3fcd3d76192e4007dfb496cca67e13b");
		compare("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789",
			"d174ab98d277d9f5a5611c2c9f419d9f");
		compare("12345678901234567890123456789012345678901234567890123456789012345678901234567890",
			"57edf4a22be3c955ac49da2e2107b67a");
	}

	void test_align()
	{
		// Make sure it's not sensitive to alignment
		const char* a0 = "a";
		const char* a1 = "?a";
		const char* a2 = "??a";
		const char* a3 = "???a";
		compare(a0+0, "0cc175b9c0f1b6a831c399e269772661");
		compare(a1+1, "0cc175b9c0f1b6a831c399e269772661");
		compare(a2+2, "0cc175b9c0f1b6a831c399e269772661");
		compare(a3+3, "0cc175b9c0f1b6a831c399e269772661");
	}

	void test_align_long()
	{
		// Make sure it's not sensitive to alignment
		// when processing long chunks (where it won't memcpy to an intermediate buffer)
		std::string a0 (1000, 'a');
		std::string a1 ("?" + a0);
		std::string a2 ("??" + a0);
		std::string a3 ("???" + a0);
		compare(a0.c_str()+0, "cabe45dcc9ae5b66ba86600cca6b8ba8");
		compare(a1.c_str()+1, "cabe45dcc9ae5b66ba86600cca6b8ba8");
		compare(a2.c_str()+2, "cabe45dcc9ae5b66ba86600cca6b8ba8");
		compare(a3.c_str()+3, "cabe45dcc9ae5b66ba86600cca6b8ba8");
		// Matches output from:
		//   perl -e'print "a" x 1000'|openssl md5
	}

	void test_padding()
	{
		// Edge cases for padding
		compare(std::string(54, 'x').c_str(), "61ea0974c662328da964d977a8253873");
		compare(std::string(55, 'x').c_str(), "04364420e25c512fd958a70738aa8f72");
		compare(std::string(56, 'x').c_str(), "668a72d5ba17f08e62dabcafad6db14b");
		compare(std::string(57, 'x').c_str(), "693037871c4a9d3d8685018905cb530a");
	}

	void test_chunks()
	{
		u8 digest[MD5::DIGESTSIZE];

		const u8* in = (const u8*)"12345678901234567890123456789012345678901234567890123456789012345678901234567890";
		size_t len = 80;
		const char* expected = "57edf4a22be3c955ac49da2e2107b67a";

		// Process in one chunk
		{
			MD5 m;
			m.Update(in, len);
			m.Final(digest);
			TS_ASSERT_STR_EQUALS(decode(digest), expected);
		}

		// Process one byte at a time
		{
			MD5 m;
			for (size_t i = 0; i < len; ++i)
				m.Update(in+i, 1);
			m.Final(digest);
			TS_ASSERT_STR_EQUALS(decode(digest), expected);
		}

		// Zero-length updates
		{
			MD5 m;
			m.Update(in+0, 1);
			m.Update(in+1, 0);
			m.Update(in+1, len-1);
			m.Update(in+len, 0);
			m.Final(digest);
			TS_ASSERT_STR_EQUALS(decode(digest), expected);
		}

		// Split at various points
		for (size_t i = 0; i <= len; ++i)
		{
			MD5 m;
			m.Update(in, i);
			m.Update(in+i, len-i);
			m.Final(digest);
			TS_ASSERT_STR_EQUALS(decode(digest), expected);
		}
	}
};

