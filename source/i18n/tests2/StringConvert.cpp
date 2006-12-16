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
