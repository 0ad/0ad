#include "precompiled.h"

#include "StringConvert.h"

#include <assert.h>

#include <jsapi.h>

// Make JS debugging a little easier by automatically naming GC roots
#ifndef NDEBUG
// Don't simply #define NAME_ALL_GC_ROOTS, because jsapi.h is horridly broken
# define JS_AddRoot(cx, rp) JS_AddNamedRoot((cx), (rp), __FILE__)
#endif


#if SDL_BYTE_ORDER == SDL_BIG_ENDIAN
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
	if( !len )
		return( JSVAL_TO_STRING( JS_GetEmptyStringValue( cx ) ) );
	jschar* data = (jschar*)JS_malloc(cx, len*sizeof(jschar));
	if (!data)
		return NULL;
	for (size_t i=0; i<len; ++i)
		data[i] = chars[i];
	return JS_NewUCString(cx, data, len);
}

void StringConvert::jsstring_to_wstring(JSString* str, std::wstring& result)
{
	jschars_to_wstring(JS_GetStringChars(str), JS_GetStringLength(str), result);
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
