/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FUString.h"
#ifndef __APPLE__
#include "FUStringBuilder.hpp"
#endif
#include <limits>

//
// FUString
//

const char* emptyCharString = "";
const fchar* emptyFCharString = FC("");
const fm::string emptyString(emptyCharString);
const fstring emptyFString(emptyFCharString);

//
// FUStringBuilder [parasitic]
//

template<> fstring FUStringBuilder::ToString() const
{
	return fstring(ToCharPtr());
}

template<> void FUStringBuilder::append(uint32 i)
{
	fchar sz[128];
	fsnprintf(sz, 128, FC("%u"), (unsigned int) i);
	append(sz);
}

template<> void FUStringBuilder::append(uint64 i)
{
	fchar sz[128];
	fsnprintf(sz, 128, FC("%u"), (unsigned int) i);
	append(sz);
}

template<> void FUStringBuilder::append(int32 i)
{
	fchar sz[128];
	fsnprintf(sz, 128, FC("%i"), (int) i);
	append(sz);
}

#ifdef UNICODE
template<> fm::string FUSStringBuilder::ToString() const
{
	return fm::string(ToCharPtr());
}

template<> void FUSStringBuilder::append(uint32 i)
{
	char sz[128];
	snprintf(sz, 128, "%u", (unsigned int) i);
	append(sz);
}

template<> void FUSStringBuilder::append(uint64 i)
{
	char sz[128];
	snprintf(sz, 128, "%u", (unsigned int) i);
	append(sz);
}

template<> void FUSStringBuilder::append(int32 i)
{
	char sz[128];
	snprintf(sz, 128, "%i", (int) i);
	append(sz);
}
#endif // UNICODE

FCOLLADA_EXPORT void TrickLinker2()
{
	{
		// Exercise ALL the functions of the string builders in order to force their templatizations.
		fm::string s = "1";
		fstring fs = FC("2");
		FUSStringBuilder b1, b2(s), b3(s.c_str()), b4('c', 3), b5(333);
		FUStringBuilder d1, d2(fs), d3(fs.c_str()), d4('c', 4), d5(333);

		b1.clear(); d1.clear(); b1.length(); d1.length();
		b1.append(s); d1.append(fs);
		b1.append('c'); d1.append((fchar) 'c');
		b1.append(b2); d1.append(d2);
		b1.append((int)1); d1.append((int)1);
		b1.append(s.c_str(), 1); d1.append(fs.c_str(), 1);
		b3.append((uint32) 8); d3.append((uint32) 8);
		b4.append((uint64) 8); d4.append((uint64) 8);
		b4.append((int32) -2); d4.append((int32) -2);
		b5.append(1.0f); d5.append(-1.0f);
		b5.append(1.0); d5.append(-1.0);
		b5.append(FMVector2::Zero); d5.append(FMVector2::Zero);
		b5.append(FMVector3::Zero); d5.append(FMVector3::Zero);
		b5.append(FMVector4::Zero); d5.append(FMVector4::Zero);
		b5.back(); d5.back();
		b1.appendLine("Test"); d1.appendLine(FC("Test"));
		b1.appendHex((uint8) 4); d1.appendHex((uint8) 4);
		b1.remove(0); d1.remove(0);
		b2.remove(0, 2); d2.remove(0, 2);
		b3.ToCharPtr(); d3.ToCharPtr();
		b4.index('c'); d4.index('c');
		b4.rindex('c'); d4.rindex('c');
	}

	{
		// Similarly for fm::stringT.
		fm::string a, b(a), c("a"), d("ab", 1), e(3, 'C');
		fstring r, s(r), t(FC("S")), u(FC("SSS"), 1), v(3, 'S');

		size_t x = a.length(), y = r.length();
		a = c.substr(x, y); r = u.substr(x, y);
		a.append("TEST"); r.append(FC("TEST"));
		a.append(d); r.append(v);
		e = a.c_str(); v = u.c_str();
		x = a.find('C', y); y = v.find('S', x);
		x = a.find(d, y); y = u.find(r, x);
		x = a.find("X", y); y = v.find(FC("A"), x);
		x = a.rfind('C', y); y = v.rfind('S', x);
		x = a.rfind(d, y); y = u.rfind(r, x);
		x = a.rfind("X", y); y = v.rfind(FC("A"), x);
		x = a.find_first_of("XSA", y); y = r.find_first_of(FC("DNA"), x);
		x = a.find_last_of("XSA", y); y = r.find_last_of(FC("DNA"), x);
		a.insert(x, "XX"); r.insert(y, FC("XX"));
		a.insert(y, c); r.insert(x, v);
		a.erase(0, fm::string::npos); t.erase(1, fstring::npos);
		a.append('c'); v.append('w');
	}

	extern void TrickLinkerFUUniqueStringMap();
	TrickLinkerFUUniqueStringMap();
	extern void TrickLinkerFUStringConversion();
	TrickLinkerFUStringConversion();
}
