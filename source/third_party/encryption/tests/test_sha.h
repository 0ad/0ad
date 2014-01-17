/* Copyright (C) 2014 Wildfire Games.
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

#include "third_party/encryption/sha.h"

class TestEncryptionSha256 : public CxxTest::TestSuite 
{
public:
	void test_sha256()
	{
		// Test two hashes of the same variable are equal.
		SHA256 hash1, hash2;
		unsigned char finalHash1A[SHA_DIGEST_SIZE], finalHash1B[SHA_DIGEST_SIZE];
		const char cStringToHash1[] = "Hash me!";
		hash1.update(cStringToHash1, sizeof(cStringToHash1));
		hash2.update(cStringToHash1, sizeof(cStringToHash1));

		hash1.finish(finalHash1A);
		hash2.finish(finalHash1B);
		TS_ASSERT_EQUALS(*finalHash1A, *finalHash1B);

		// Test that the output isn't the same as the input.
		TS_ASSERT_DIFFERS(*cStringToHash1, *finalHash1A)

		// Test if updating the hash multiple times changes the
		//  original hashes but still results in them being equal.
		unsigned char finalHash2A[SHA_DIGEST_SIZE], finalHash2B[SHA_DIGEST_SIZE];
		const char cStringToHash2[] = "Hash me too please!";
		hash1.update(cStringToHash2, sizeof(cStringToHash2));
		hash2.update(cStringToHash2, sizeof(cStringToHash2));

		hash1.finish(finalHash2A);
		hash2.finish(finalHash2B);
		TS_ASSERT_EQUALS(*finalHash2A, *finalHash2B);

		// Make sure the updated hash is actually different
		//  compared to the original hash.
		TS_ASSERT_DIFFERS(*finalHash1A, *finalHash2A);
		TS_ASSERT_DIFFERS(*finalHash1B, *finalHash2B);
	}
};
