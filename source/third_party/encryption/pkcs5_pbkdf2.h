/* Copyright (C) 2018 Wildfire Games.
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

#ifndef PKCS5_PBKD2_INCLUDED
#define PKCS5_PBKD2_INCLUDED

#include <sodium.h>

/**
 * Simple PBKDF2 implementation for hard to crack passwords
 * @param output The output buffer for the digested hash
 * @param key The initial key we want to hash
 * @param key_len Length of the key in bytes
 * @param salt The salt we use to iteratively hash the key.
 * @param salt_len Length of the salt in bytes
 * @param iterations Number of salting iterations
 * @return 0 on success, -1 on error
 */
int pbkdf2(unsigned char (&output)[crypto_hash_sha256_BYTES],
			const unsigned char* key, size_t key_len,
			const unsigned char* salt, size_t salt_len,
			unsigned iterations);

#endif // PKCS5_PBKD2_INCLUDED
