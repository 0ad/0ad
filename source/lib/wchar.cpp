/* Copyright (C) 2009 Wildfire Games.
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
#include "wchar.h"

// adapted from http://unicode.org/Public/PROGRAMS/CVTUTF/ConvertUTF.c
// which bears the following notice:
/*
* Copyright 2001-2004 Unicode, Inc.
* 
* Disclaimer
* 
* This source code is provided as is by Unicode, Inc. No claims are
* made as to fitness for any particular purpose. No warranties of any
* kind are expressed or implied. The recipient agrees to determine
* applicability of information provided. If this file has been
* purchased on magnetic or optical media from Unicode, Inc., the
* sole remedy for any claim will be exchange of defective media
* within 90 days of receipt.
* 
* Limitations on Rights to Redistribute This Code
* 
* Unicode, Inc. hereby grants the right to freely use the information
* supplied in this file in the creation of products supporting the
* Unicode Standard, and to make copies of this file in any form
* for internal or external distribution as long as this notice
* remains attached.
*/

// design rationale:
// - to cope with wchar_t differences between VC (UTF-16) and
//   GCC (UCS-4), we only allow codepoints in the BMP.
//   UTF-8 encodings are therefore no longer than 3 bytes.
// - surrogates are disabled because variable-length strings
//   violate the purpose of using wchar_t instead of UTF-8.

// this implementation survives http://www.cl.cam.ac.uk/~mgk25/ucs/examples/UTF-8-test.txt

// (must be unsigned to avoid sign extension)
typedef u8 UTF8;
typedef u32 UTF32;

static const UTF32 replacementCharacter = 0xFFFDul;	// used by ReplaceIfInvalid and UTF8::Decode

static UTF32 ReplaceIfInvalid(UTF32 u)
{
	// disallow surrogates
	if(0xD800ul <= u && u <= 0xDFFFul)
		return replacementCharacter;
	// 0xFFFE:  byte order marker (invalid character)
	// 0xFFFF:  permanently unassigned code point, may correspond to WEOF
	//          (raises errors when used in VC's swprintf)
	// greater: UTF-16 representation would require surrogates
	if(u >= 0xFFFEul)	
		return replacementCharacter;
	return u;
}


class UTF8Codec
{
public:
	static void Encode(UTF32 u, UTF8*& dstPos)
	{
		const size_t size = Size(u);
		static const UTF8 firstByteMarks[1+3] = { 0, 0x00, 0xC0, 0xE0 };
		for(size_t i = 1; i < size; i++)
		{
			dstPos[size-i] = UTF8((u|0x80u) & 0xBFu);
			u >>= 6;
		}
		dstPos[0] = UTF8(u | firstByteMarks[size]);
		dstPos += size;
	}

	// @return decoded scalar, or replacementCharacter on error
	static UTF32 Decode(const UTF8*& srcPos, const UTF8* const srcEnd)
	{
		const size_t size = SizeFromFirstByte(*srcPos);
		if(!IsValid(srcPos, size, srcEnd))
		{
			srcPos += 1;	// only skip the offending byte (increases chances of resynchronization)
			return replacementCharacter;
		}

		UTF32 u = 0;
		for(size_t i = 0; i < size-1; i++)
		{
			u += UTF32(*srcPos++);
			u <<= 6;
		}
		u += UTF32(*srcPos++);

		static const UTF32 offsets[1+4] = { 0, 0x00000000ul, 0x00003080ul, 0x000E2080ul, 0x03C82080UL };
		u -= offsets[size];
		return u;
	}

private:
	static inline size_t Size(UTF32 u)
	{
		if(u < 0x80)
			return 1;
		if(u < 0x800)
			return 2;
		// ReplaceIfInvalid ensures > 3 byte encodings are never used.
		return 3;
	}

	static inline size_t SizeFromFirstByte(UTF8 firstByte)
	{
		if(firstByte < 0xC0)
			return 1;
		if(firstByte < 0xE0)
			return 2;
		if(firstByte < 0xF0)
			return 3;
		// IsValid rejects firstByte values that would cause > 4 byte encodings.
		return 4;
	}

	// c.f. Unicode 3.1 Table 3-7
	// @param size obtained via SizeFromFirstByte (our caller also uses it)
	static bool IsValid(const UTF8* const src, size_t size, const UTF8* const srcEnd)
	{
		if(src+size > srcEnd)	// not enough data
			return false;

		if(src[0] < 0x80)
			return true;
		if(!(0xC2 <= src[0] && src[0] <= 0xF4))
			return false;

		// special cases (stricter than the loop)
		if(src[0] == 0xE0 && src[1] < 0xA0)
			return false;
		if(src[0] == 0xED && src[1] > 0x9F)
			return false;
		if(src[0] == 0xF0 && src[1] < 0x90)
			return false;
		if(src[0] == 0xF4 && src[1] > 0x8F)
			return false;

		for(size_t i = 1; i < size; i++)
		{
			if(!(0x80 <= src[i] && src[i] <= 0xBF))
				return false;
		}

		return true;
	}
};


//-----------------------------------------------------------------------------

std::string utf8_from_wstring(const std::wstring& src)
{
	std::string dst(src.size()*3+1, ' ');	// see UTF8Codec::Size; +1 ensures &dst[0] is valid
	UTF8* dstPos = (UTF8*)&dst[0];
	for(size_t i = 0; i < src.size(); i++)
	{
		const UTF32 u = ReplaceIfInvalid(UTF32(src[i]));
		UTF8Codec::Encode(u, dstPos);
	}
	dst.resize(dstPos - (UTF8*)&dst[0]);
	return dst;
}


std::wstring wstring_from_utf8(const std::string& src)
{
	std::wstring dst;
	dst.reserve(src.size());
	const UTF8* srcPos = (const UTF8*)src.data();
	const UTF8* const srcEnd = srcPos + src.size();
	while(srcPos < srcEnd)
	{
		const UTF32 u = UTF8Codec::Decode(srcPos, srcEnd);
		dst.push_back((wchar_t)ReplaceIfInvalid(u));
	}
	return dst;
}
