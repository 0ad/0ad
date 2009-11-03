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

#include "BufferVariable.h"

#include "DataTypes.h"
#include "CLocale.h"

// Hashing functions are currently 32-bit only
cassert(sizeof(int) == 4);
cassert(sizeof(double) == 8);
// (TODO: the hashing here is quite rubbish)

using namespace I18n;

namespace I18n {
	// These bits don't seem to work without the explicit namespace{}

	template<> BufferVariable* NewBufferVariable<int>(int v) { return new BufferVariable_int(v); }
	template<> BufferVariable* NewBufferVariable<float>(float v) { return new BufferVariable_double(v); }
	template<> BufferVariable* NewBufferVariable<double>(double v) { return new BufferVariable_double(v); }
	template<> BufferVariable* NewBufferVariable<Noun>(Noun v) { return new BufferVariable_string(v.value); }
	template<> BufferVariable* NewBufferVariable<Name>(Name v) { return new BufferVariable_rawstring(v.value); }

	//
	// Don't allow plain strings -- people *must* specify whether it's going
	// to be automatically translated (I18n::Noun) or not (I18n::Name/Raw)
	//
}

StrImW BufferVariable_int::ToString(CLocale*)
{
	wchar_t buffer[16];
	swprintf(buffer, 16, L"%d", value);
	return buffer;
}
u32 BufferVariable_int::Hash()
{
	return (u32)value;
}


StrImW BufferVariable_double::ToString(CLocale*)
{
	// TODO: Work out how big the buffer should be
	wchar_t buffer[256];
	swprintf(buffer, 256, L"%f", value);
	return buffer;
}
u32 BufferVariable_double::Hash()
{
	// Add the two four-bytes of the double
	union {
		u32 i[2];
		double d;
	} u;
	u.d = value;
	return u.i[0]+u.i[1];
}


StrImW BufferVariable_string::ToString(CLocale* locale)
{
	// Attempt noun translations
	const Str nouns = L"nouns";
	const Str wordname (value.str());
	const CLocale::LookupType* word = locale->LookupWord(nouns, wordname);
	// Couldn't find; just return the original word
	if (! word)
		return value;

	const Str propname = L"singular";
	Str ret;
	if (! locale->LookupProperty(word, propname, ret))
	{
		delete word;
		return value;
	}
	delete word;
	return ret.c_str();
}
u32 BufferVariable_string::Hash()
{
	// Do nothing very clever
	int hash = 0;
	const wchar_t* str = value.str();
	size_t len = wcslen(str);
	for (size_t i = 0; i < len; ++i)
		hash += str[i];
	return hash;
}

StrImW BufferVariable_rawstring::ToString(CLocale*)
{
	return value;
}
u32 BufferVariable_rawstring::Hash()
{
	int hash = 0;
	const wchar_t* str = value.str();
	size_t len = wcslen(str);
	for (size_t i = 0; i < len; ++i)
		hash += str[i];
	return hash;
}

bool I18n::operator== (BufferVariable& a, BufferVariable& b)
{
	if (a.Type != b.Type)
		return false;

	switch (a.Type)
	{
	case vartype_int:
		return *static_cast<BufferVariable_int*>(&a) == *static_cast<BufferVariable_int*>(&b);
	case vartype_double:
		return *static_cast<BufferVariable_double*>(&a) == *static_cast<BufferVariable_double*>(&b);
	case vartype_string:
		return *static_cast<BufferVariable_string*>(&a) == *static_cast<BufferVariable_string*>(&b);
	case vartype_rawstring:
		return *static_cast<BufferVariable_rawstring*>(&a) == *static_cast<BufferVariable_rawstring*>(&b);
	}
	debug_warn(L"Invalid buffer variable vartype");
	return false;
}

bool BufferVariable_int::operator== (BufferVariable_int& a)
{
	return a.value == this->value;
}

bool BufferVariable_double::operator== (BufferVariable_double& a)
{
	return a.value == this->value;
}

bool BufferVariable_string::operator== (BufferVariable_string& a)
{
	return a.value == this->value;
}

bool BufferVariable_rawstring::operator== (BufferVariable_rawstring& a)
{
	return a.value == this->value;
}
