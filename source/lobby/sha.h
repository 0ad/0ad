/* Copyright (c) 2013 Wildfire Games
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

#ifndef SHA_INCLUDED
#define SHA_INCLUDED

#define SHA_DIGEST_SIZE 32
typedef unsigned char byte;
typedef unsigned int uint;


/**
 * Structure for performing SHA256 encryption on arbitrary data
 */
struct SHA256
{
	uint total[2];
	uint state[8];
	byte buffer[64];
 
	SHA256();
	void init();
	void transform(byte (&data)[64]);
	void update(const void* input, uint len);
	void finish(byte (&digest)[32]);
};
 
 
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
int pbkdf2(byte (&output)[SHA_DIGEST_SIZE],
			const byte* key, size_t key_len,
			const byte* salt, size_t salt_len,
			unsigned iterations);
 
#endif // SHA_INCLUDED
