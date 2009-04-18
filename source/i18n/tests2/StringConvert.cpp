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

#include "StringConvert.h"

#include <assert.h>
#include "jsapi.h"

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
#define ucs2le_to_wchart(ptr) (wchar_t)( (u16) ((u8*)ptr)[0] | (u16) ( ((u8*)ptr)[1] << 8) )
#else
#define ucs2le_to_wchart(ptr) (wchar_t)(*ptr);
#endif

JSString* StringConvert::wstring_to_jsstring(JSContext* cx, const std::wstring& str)
{
	size_t len = str.length();
	jschar* data = (jschar*)JS_malloc(cx, len*sizeof(jschar));
	if (!data)
		return NULL;
	for (size_t i=0; i<len; ++i)
		data[i] = str[i];
	return JS_NewUCString(cx, data, len);
}

JSString* StringConvert::wchars_to_jsstring(JSContext* cx, const wchar_t* chars)
{
	size_t len = wcslen(chars);
	jschar* data = (jschar*)JS_malloc(cx, len*sizeof(jschar));
	if (!data)
		return NULL;
	for (size_t i=0; i<len; ++i)
		data[i] = chars[i];
	return JS_NewUCString(cx, data, len);
}

void StringConvert::jschars_to_wstring(const jschar* chars, size_t len, std::wstring& result)
{
	assert(result.empty());
	result.reserve(len);

	for (size_t i = 0; i < len; ++i)
		result += chars[i];
}


void StringConvert::ucs2le_to_wstring(const char* start, const char* end, std::wstring& result)
{
	assert(result.empty());
	result.reserve(end-start);

	for (const char* pos = start; pos < end; pos += 2)
		result += ucs2le_to_wchart(pos);
}
