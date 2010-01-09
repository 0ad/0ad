/* Copyright (C) 2010 Wildfire Games.
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

#ifndef INCLUDED_HASHSERIALIZER
#define INCLUDED_HASHSERIALIZER

#include "BinarySerializer.h"

#include "lib/external_libraries/cryptopp.h"

class CHashSerializer : public CBinarySerializer
{
	// We don't care about cryptographic strength, just about detection of
	// unintended changes and about performance, but we do want to use an algorithm
	// that's included in Crypto++'s Windows DLL, so SHA1 is an adequate choice
	typedef CryptoPP::SHA1 HashFunc;
public:
	CHashSerializer(ScriptInterface& scriptInterface);
	size_t GetHashLength();
	const u8* ComputeHash();

protected:
	virtual void Put(const char* name, const u8* data, size_t len);

private:
	HashFunc m_Hash;
	u8 m_HashData[HashFunc::DIGESTSIZE];
};

#endif // INCLUDED_HASHSERIALIZER
