/**
 * Copyright (c) 2008 Damien Bergamini <damien.bergamini@free.fr>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
// This file is based loosly off libutil/pkcs5_pbkdf2.c in OpenBSD.

#include "precompiled.h"

#include "pkcs5_pbkdf2.h"
#include "sha.h"

static void hmac_sha256(unsigned char (&digest)[SHA_DIGEST_SIZE],
						const unsigned char* text, size_t text_len,
						const unsigned char* key, size_t key_len)
{
	SHA256 hash;
	unsigned char tk[SHA_DIGEST_SIZE]; // temporary key incase we need to pad the key with zero unsigned chars
	if (key_len > SHA_DIGEST_SIZE)
	{
		hash.update(key, key_len);
		hash.finish(tk);
		key = tk;
		key_len = SHA_DIGEST_SIZE;
	}
 
	unsigned char k_pad[SHA_DIGEST_SIZE];
 
	memset(k_pad, 0, sizeof k_pad);
	memcpy(k_pad, key, key_len);
	for (int i = 0; i < SHA_DIGEST_SIZE; ++i)
		k_pad[i] ^= 0x36;
	hash.init();
	hash.update(k_pad, SHA_DIGEST_SIZE);
	hash.update(text, text_len);
	hash.finish(digest);
 
 
	memset(k_pad, 0, sizeof k_pad);
	memcpy(k_pad, key, key_len);
	for (int i = 0; i < SHA_DIGEST_SIZE; ++i)
		k_pad[i] ^= 0x5c;
 
	hash.init();
	hash.update(k_pad, SHA_DIGEST_SIZE);
	hash.update(digest, SHA_DIGEST_SIZE);
	hash.finish(digest);
}
 
 
int pbkdf2(unsigned char (&output)[SHA_DIGEST_SIZE],
			const unsigned char* key, size_t key_len,
			const unsigned char* salt, size_t salt_len,
			unsigned rounds)
{
	unsigned char asalt[SHA_DIGEST_SIZE + 4], obuf[SHA_DIGEST_SIZE], d1[SHA_DIGEST_SIZE], d2[SHA_DIGEST_SIZE];
 
	if (rounds < 1 || key_len == 0 || salt_len == 0)
		return -1;
 
	if (salt_len > SHA_DIGEST_SIZE) salt_len = SHA_DIGEST_SIZE; // length cap for the salt
	memset(asalt, 0, salt_len);
	memcpy(asalt, salt, salt_len);
 
	for (unsigned count = 1; ; ++count)
	{
		asalt[salt_len + 0] = (count >> 24) & 0xff;
		asalt[salt_len + 1] = (count >> 16) & 0xff;
		asalt[salt_len + 2] = (count >> 8) & 0xff;
		asalt[salt_len + 3] = count & 0xff;
		hmac_sha256(d1, asalt, salt_len + 4, key, key_len);
		memcpy(obuf, d1, SHA_DIGEST_SIZE);
 
		for (unsigned i = 1; i < rounds; i++)
		{
			hmac_sha256(d2, d1, SHA_DIGEST_SIZE, key, key_len);
			memcpy(d1, d2, SHA_DIGEST_SIZE);
			for (unsigned j = 0; j < SHA_DIGEST_SIZE; j++)
				obuf[j] ^= d1[j];
		}
 
		memcpy(output, obuf, SHA_DIGEST_SIZE);
		key += SHA_DIGEST_SIZE;
		if (key_len < SHA_DIGEST_SIZE)
			break;
		key_len -= SHA_DIGEST_SIZE;
	};
	return 0;
}

