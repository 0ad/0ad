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
#include "lib/external_libraries/sdl.h"
#include "scripting/SpiderMonkey.h"

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
#define ucs2le_to_wchart(ptr) (wchar_t)( (u16) ((u8*)ptr)[0] | (u16) ( ((u8*)ptr)[1] << 8) )
#else
#define ucs2le_to_wchart(ptr) (wchar_t)(*ptr);
#endif


JSString* StringConvert::wchars_to_jsstring(JSContext* cx, const wchar_t* chars, size_t len)
{
	if (len == 0)
		return JSVAL_TO_STRING(JS_GetEmptyStringValue(cx));

	jschar* data = (jschar*)JS_malloc(cx, len*sizeof(jschar));

	if (!data)
		return NULL;

	// Copy the string data, doing wchar_t->jschar conversion (since they're not equal in GCC)
	for (size_t i=0; i<len; ++i)
		data[i] = chars[i];

	return JS_NewUCString(cx, data, len);
}

JSString* StringConvert::wchars_to_jsstring(JSContext* cx, const wchar_t* chars)
{
	return wchars_to_jsstring(cx, chars, wcslen(chars));
}

JSString* StringConvert::wstring_to_jsstring(JSContext* cx, const std::wstring& str)
{
	return wchars_to_jsstring(cx, str.data(), str.length());
}



void StringConvert::jsstring_to_wstring(JSString* str, std::wstring& result)
{
	jschars_to_wstring(JS_GetStringChars(str), JS_GetStringLength(str), result);
}

void StringConvert::jschars_to_wstring(const jschar* chars, size_t len, std::wstring& result)
{
	debug_assert(result.empty());
	result.resize(len);

	for (size_t i = 0; i < len; ++i)
		result[i] = chars[i];
}


void StringConvert::ucs2le_to_wstring(const char* start, const char* end, std::wstring& result)
{
	debug_assert(result.empty());
	result.resize((end-start)/2);

	size_t i = 0;
	for (const char* pos = start; pos < end; pos += 2)
		result[i++] = ucs2le_to_wchart(pos);
}
