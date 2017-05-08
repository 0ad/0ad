/* Copyright (c) 2017 Wildfire Games
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

#include "precompiled.h"

#include "lib/utf8.h"

static const StatusDefinition utf8StatusDefinitions[] = {
	{ ERR::UTF8_SURROGATE, L"UTF-16 surrogate pairs aren't supported" },
	{ ERR::UTF8_OUTSIDE_BMP, L"Code point outside BMP (> 0x10000)" },
	{ ERR::UTF8_NONCHARACTER, L"Noncharacter (e.g. WEOF)" },
	{ ERR::UTF8_INVALID_UTF8, L"Invalid UTF-8 sequence" }
};
STATUS_ADD_DEFINITIONS(utf8StatusDefinitions);


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
//   encoded UTF-8 sequences are therefore no longer than 3 bytes.
// - surrogates are disabled because variable-length strings
//   violate the purpose of using wchar_t instead of UTF-8.
// - replacing disallowed characters instead of aborting outright
//   avoids overly inconveniencing users and eases debugging.

// this implementation survives http://www.cl.cam.ac.uk/~mgk25/ucs/examples/UTF-8-test.txt

// (must be unsigned to avoid sign extension)
typedef u8 UTF8;
typedef u32 UTF32;


// called from ReplaceIfInvalid and UTF8Codec::Decode
static UTF32 RaiseError(Status err, Status* perr)
{
	if(perr)	// caller wants return code, not warning dialog
	{
		if(*perr == INFO::OK)	// only return the first error (see header)
			*perr = err;
	}
	else
	{
		wchar_t error[200];
		debug_printf("UTF8 error: %s\n", utf8_from_wstring(StatusDescription(err, error, ARRAY_SIZE(error))).c_str());
	}

	return 0xFFFDul;	// replacement character
}


static UTF32 ReplaceIfInvalid(UTF32 u, Status* err)
{
	// disallow surrogates
	if(0xD800ul <= u && u <= 0xDFFFul)
		return RaiseError(ERR::UTF8_SURROGATE, err);
	// outside BMP (UTF-16 representation would require surrogates)
	if(u > 0xFFFFul)
		return RaiseError(ERR::UTF8_OUTSIDE_BMP, err);
	// noncharacter (note: WEOF (0xFFFF) causes VC's swprintf to fail)
	if(u == 0xFFFEul || u == 0xFFFFul || (0xFDD0ul <= u && u <= 0xFDEFul))
		return RaiseError(ERR::UTF8_NONCHARACTER, err);
	return u;
}


class UTF8Codec
{
public:
	static void Encode(UTF32 u, UTF8*& dstPos)
	{
		switch (Size(u))
		{
		case 1:
			*dstPos++ = UTF8(u);
			break;
		case 2:
			*dstPos++ = UTF8((u >> 6) | 0xC0);
			*dstPos++ = UTF8((u | 0x80u) & 0xBFu);
			break;
		case 3:
			*dstPos++ = UTF8((u >> 12) | 0xE0);
			*dstPos++ = UTF8(((u >> 6) | 0x80u) & 0xBFu);
			*dstPos++ = UTF8((u | 0x80u) & 0xBFu);
			break;
		}
	}

	// @return decoded scalar, or replacementCharacter on error
	static UTF32 Decode(const UTF8*& srcPos, const UTF8* const srcEnd, Status* err)
	{
		const size_t size = SizeFromFirstByte(*srcPos);
		if(!IsValid(srcPos, size, srcEnd))
		{
			srcPos += 1;	// only skip the offending byte (increases chances of resynchronization)
			return RaiseError(ERR::UTF8_INVALID_UTF8, err);
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

std::string utf8_from_wstring(const std::wstring& src, Status* err)
{
	if(err)
		*err = INFO::OK;

	std::string dst(src.size()*3+1, ' ');	// see UTF8Codec::Size; +1 ensures &dst[0] is valid
	UTF8* dstPos = (UTF8*)&dst[0];
	for(size_t i = 0; i < src.size(); i++)
	{
		const UTF32 u = ReplaceIfInvalid(UTF32(src[i]), err);
		UTF8Codec::Encode(u, dstPos);
	}
	dst.resize(dstPos - (UTF8*)&dst[0]);
	return dst;
}


std::wstring wstring_from_utf8(const std::string& src, Status* err)
{
	if(err)
		*err = INFO::OK;

	std::wstring dst;
	dst.reserve(src.size());
	const UTF8* srcPos = (const UTF8*)src.data();
	const UTF8* const srcEnd = srcPos + src.size();
	while(srcPos < srcEnd)
	{
		const UTF32 u = UTF8Codec::Decode(srcPos, srcEnd, err);
		dst.push_back((wchar_t)ReplaceIfInvalid(u, err));
	}
	return dst;
}
