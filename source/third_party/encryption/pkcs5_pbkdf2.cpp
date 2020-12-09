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

#include <cstring>

// This does not match libsodium crypto_auth_hmacsha256, which has a constant key_len.
static void hmac_sha256(unsigned char (&digest)[crypto_hash_sha256_BYTES],
						const unsigned char* text, size_t text_len,
						const unsigned char* key, size_t key_len)
{
	crypto_hash_sha256_state state;
	crypto_hash_sha256_init(&state);

	unsigned char tk[crypto_hash_sha256_BYTES]; // temporary key in case we need to pad the key with zero unsigned chars
	if (key_len > crypto_hash_sha256_BYTES)
	{
		crypto_hash_sha256_update(&state, key, key_len);
		crypto_hash_sha256_final(&state, tk);
		key = tk;
		key_len = crypto_hash_sha256_BYTES;
	}

	unsigned char k_pad[crypto_hash_sha256_BYTES];

	memset(k_pad, 0, sizeof k_pad);
	memcpy(k_pad, key, key_len);
	for (unsigned int i = 0; i < crypto_hash_sha256_BYTES; ++i)
		k_pad[i] ^= 0x36;
	crypto_hash_sha256_init(&state);
	crypto_hash_sha256_update(&state, k_pad, crypto_hash_sha256_BYTES);
	crypto_hash_sha256_update(&state, text, text_len);
	crypto_hash_sha256_final(&state, digest);


	memset(k_pad, 0, sizeof k_pad);
	memcpy(k_pad, key, key_len);
	for (unsigned int i = 0; i < crypto_hash_sha256_BYTES; ++i)
		k_pad[i] ^= 0x5c;

	crypto_hash_sha256_init(&state);
	crypto_hash_sha256_update(&state, k_pad, crypto_hash_sha256_BYTES);
	crypto_hash_sha256_update(&state, digest, crypto_hash_sha256_BYTES);
	crypto_hash_sha256_final(&state, digest);
}


int pbkdf2(unsigned char (&output)[crypto_hash_sha256_BYTES],
			const unsigned char* key, size_t key_len,
			const unsigned char* salt, size_t salt_len,
			unsigned rounds)
{
	unsigned char asalt[crypto_hash_sha256_BYTES + 4], obuf[crypto_hash_sha256_BYTES], d1[crypto_hash_sha256_BYTES], d2[crypto_hash_sha256_BYTES];

	if (rounds < 1 || key_len == 0 || salt_len == 0)
		return -1;

	if (salt_len > crypto_hash_sha256_BYTES) salt_len = crypto_hash_sha256_BYTES; // length cap for the salt
	memset(asalt, 0, salt_len);
	memcpy(asalt, salt, salt_len);

	for (unsigned count = 1; ; ++count)
	{
		asalt[salt_len + 0] = (count >> 24) & 0xff;
		asalt[salt_len + 1] = (count >> 16) & 0xff;
		asalt[salt_len + 2] = (count >> 8) & 0xff;
		asalt[salt_len + 3] = count & 0xff;
		hmac_sha256(d1, asalt, salt_len + 4, key, key_len);
		memcpy(obuf, d1, crypto_hash_sha256_BYTES);

		for (unsigned i = 1; i < rounds; i++)
		{
			hmac_sha256(d2, d1, crypto_hash_sha256_BYTES, key, key_len);
			memcpy(d1, d2, crypto_hash_sha256_BYTES);
			for (unsigned j = 0; j < crypto_hash_sha256_BYTES; j++)
				obuf[j] ^= d1[j];
		}

		memcpy(output, obuf, crypto_hash_sha256_BYTES);
		key += crypto_hash_sha256_BYTES;
		if (key_len < crypto_hash_sha256_BYTES)
			break;
		key_len -= crypto_hash_sha256_BYTES;
	};
	return 0;
}

