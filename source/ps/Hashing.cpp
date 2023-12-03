/* Copyright (C) 2021 Wildfire Games.
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
#include "precompiled.h"

#include "ps/CStr.h"
#include "ps/Util.h"

#include <sodium.h>

CStr8 HashCryptographically(const CStr8& string, const CStr8& salt)
{
	if (string.empty())
		return string;

	ENSURE(sodium_init() >= 0);

	constexpr int SALTSIZE = crypto_pwhash_SALTBYTES;
	static_assert(SALTSIZE >= crypto_generichash_BYTES_MIN);
	static_assert(SALTSIZE <= crypto_generichash_BYTES_MAX);
	static_assert(SALTSIZE >= crypto_generichash_KEYBYTES_MIN);
	static_assert(SALTSIZE <= crypto_generichash_KEYBYTES_MAX);

	// First generate a fixed-size salt from out variable-sized one (libsodium requires it).
	unsigned char salt_buffer[SALTSIZE] = {
		235, 82, 29, 20, 135, 168, 184, 97, 7, 240, 48, 109, 8, 34, 158, 32,
	};
	crypto_generichash_state state;
	crypto_generichash_init(&state, salt_buffer, SALTSIZE, SALTSIZE);
	crypto_generichash_update(&state, reinterpret_cast<const unsigned char*>(salt.c_str()), salt.size());
	crypto_generichash_final(&state, salt_buffer, SALTSIZE);

	constexpr int HASHSIZE = 32;
	static_assert(HASHSIZE >= crypto_pwhash_BYTES_MIN);
	static_assert(HASHSIZE <= crypto_pwhash_BYTES_MAX);

	// Now that we have a fixed-length key, use that to hash the password.
	unsigned char output[HASHSIZE] = { 0 };
	// For HashCryptographically, we use 'fast' parameters, corresponding to low values.
	// These parameters must not change, or hashes will change, hence why the #defined values are copied.
	constexpr size_t memLimit = 8192 * 4; // 4 * crypto_pwhash_argon2id_MEMLIMIT_MIN
	constexpr size_t opsLimit = 2; // crypto_pwhash_argon2id_OPSLIMIT_INTERACTIVE
	ENSURE(crypto_pwhash(output, HASHSIZE, string.c_str(), string.size(), salt_buffer, opsLimit, memLimit, crypto_pwhash_ALG_ARGON2ID13) == 0);

	return CStr(Hexify(output, HASHSIZE)).UpperCase();
}
