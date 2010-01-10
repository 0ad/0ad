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

#ifndef INCLUDED_MD5
#define INCLUDED_MD5

/**
 * MD5 hashing algorithm. Note that MD5 is broken and must not be used for
 * anything that requires security.
 */
class MD5
{
public:
	static const size_t DIGESTSIZE = 16;

	MD5();
	void Update(const u8* data, size_t len);
	void Final(u8* digest);
private:
	void InitState();
	void Transform(const u32* in);
	u32 m_Digest[4]; // internal state
	u8 m_Buf[64]; // buffered input bytes
	size_t m_BufLen; // bytes in m_Buf that are valid
	u64 m_InputLen; // bytes
};

#endif // INCLUDED_MD5
