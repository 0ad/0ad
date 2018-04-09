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

#include "third_party/encryption/pkcs5_pbkdf2.h"

class TestEncryptionPkcs5Pbkd2 : public CxxTest::TestSuite 
{
public:
	void test_pkcs5_pbkd2()
	{
		// Mock salt.
		const unsigned char salt_buffer[crypto_hash_sha256_BYTES] = {
			244, 243, 249, 244, 32, 33, 34, 35, 10, 11, 12, 13, 14, 15, 16, 17,
			18, 19, 20, 32, 33, 244, 224, 127, 129, 130, 140, 153, 133, 123, 234, 123 };
		// Mock passwords.
		const char password1[] = "0adr0ckz";
		const char password2[] = "0adIsAws0me";

		// Run twice with the same input.
		unsigned char encrypted1A[crypto_hash_sha256_BYTES], encrypted1B[crypto_hash_sha256_BYTES];
		pbkdf2(encrypted1A, (unsigned char*)password1, sizeof(password1), salt_buffer, crypto_hash_sha256_BYTES, 50);
		pbkdf2(encrypted1B, (unsigned char*)password1, sizeof(password1), salt_buffer, crypto_hash_sha256_BYTES, 50);

		// Test that the result does not equal input.
		TS_ASSERT_DIFFERS(*password1, *encrypted1A);
		TS_ASSERT_DIFFERS(*salt_buffer, *encrypted1A);

		// Test determinism.
		TS_ASSERT_EQUALS(*encrypted1A, *encrypted1B);

		// Run twice again with more iterations.
		unsigned char encrypted2A[crypto_hash_sha256_BYTES], encrypted2B[crypto_hash_sha256_BYTES];
		pbkdf2(encrypted2A, (unsigned char*)password1, sizeof(password1), salt_buffer, crypto_hash_sha256_BYTES, 100);
		pbkdf2(encrypted2B, (unsigned char*)password1, sizeof(password1), salt_buffer, crypto_hash_sha256_BYTES, 100);

		// Test determinism.
		TS_ASSERT_EQUALS(*encrypted2A, *encrypted2B);

		// Make sure more iterations results differently.
		TS_ASSERT_DIFFERS(*encrypted1A, *encrypted2A);

		// Run twice again with different password.
		unsigned char encrypted3A[crypto_hash_sha256_BYTES], encrypted3B[crypto_hash_sha256_BYTES];
		pbkdf2(encrypted3A, (unsigned char*)password2, sizeof(password2), salt_buffer, crypto_hash_sha256_BYTES, 50);
		pbkdf2(encrypted3B, (unsigned char*)password2, sizeof(password2), salt_buffer, crypto_hash_sha256_BYTES, 50);

		// Test determinism.
		TS_ASSERT_EQUALS(*encrypted3A, *encrypted3B);

		// Make sure a different password results differently.
		TS_ASSERT_DIFFERS(*encrypted3A, *encrypted1A);
	}
};
