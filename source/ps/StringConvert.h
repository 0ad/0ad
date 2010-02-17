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

#ifndef INCLUDED_STRINGCONVERT
#define INCLUDED_STRINGCONVERT

typedef unsigned short jschar;
typedef unsigned short ucs2char;

struct JSString;
struct JSContext;

#include <string>

namespace StringConvert
{
	// A random collection of conversion functions:

	JSString* wstring_to_jsstring(JSContext* cx, const std::wstring& str);
	JSString* wchars_to_jsstring(JSContext* cx, const wchar_t* chars);
	JSString* wchars_to_jsstring(JSContext* cx, const wchar_t* chars, size_t len);

	void jsstring_to_wstring(JSString* str, std::wstring& result);
	void jschars_to_wstring(const jschar* chars, size_t len, std::wstring& result);
	void ucs2le_to_wstring(const char* start, const char* end, std::wstring& result);

}

#endif // INCLUDED_STRINGCONVERT
