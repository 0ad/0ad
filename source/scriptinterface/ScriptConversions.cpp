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

#include "precompiled.h"

#include "ScriptInterface.h"

#include "ps/utf16string.h"
#include "ps/CLogger.h"

#include "js/jsapi.h"

#define FAIL(msg) STMT(JS_ReportError(cx, msg); return false)

// Implicit type conversions often hide bugs, so warn about them
#define WARN_IF_NOT(c) STMT(if (!(c)) { JS_ReportWarning(cx, "Script value conversion check failed: %s", #c); })

template<> bool ScriptInterface::FromJSVal<bool>(JSContext* cx, jsval v, bool& out)
{
	JSBool ret;
	WARN_IF_NOT(JSVAL_IS_BOOLEAN(v));
	if (!JS_ValueToBoolean(cx, v, &ret))
		return false;
	out = (ret ? true : false);
	return true;
}

template<> bool ScriptInterface::FromJSVal<float>(JSContext* cx, jsval v, float& out)
{
	jsdouble ret;
	WARN_IF_NOT(JSVAL_IS_NUMBER(v));
	if (!JS_ValueToNumber(cx, v, &ret))
		return false;
	out = ret;
	return true;
}

template<> bool ScriptInterface::FromJSVal<double>(JSContext* cx, jsval v, double& out)
{
	jsdouble ret;
	WARN_IF_NOT(JSVAL_IS_NUMBER(v));
	if (!JS_ValueToNumber(cx, v, &ret))
		return false;
	out = ret;
	return true;
}

template<> bool ScriptInterface::FromJSVal<i32>(JSContext* cx, jsval v, i32& out)
{
	int32 ret;
	WARN_IF_NOT(JSVAL_IS_INT(v));
	if (!JS_ValueToECMAInt32(cx, v, &ret))
		return false;
	out = ret;
	return true;
}

template<> bool ScriptInterface::FromJSVal<u32>(JSContext* cx, jsval v, u32& out)
{
	uint32 ret;
	WARN_IF_NOT(JSVAL_IS_INT(v));
	if (!JS_ValueToECMAUint32(cx, v, &ret))
		return false;
	out = ret;
	return true;
}

// NOTE: we can't define a jsval specialisation, because that conflicts with integer types
template<> bool ScriptInterface::FromJSVal<CScriptVal>(JSContext* UNUSED(cx), jsval v, CScriptVal& out)
{
	out = v;
	return true;
}

template<> bool ScriptInterface::FromJSVal<CScriptValRooted>(JSContext* cx, jsval v, CScriptValRooted& out)
{
	out = CScriptValRooted(cx, v);
	return true;
}

template<> bool ScriptInterface::FromJSVal<std::wstring>(JSContext* cx, jsval v, std::wstring& out)
{
	WARN_IF_NOT(JSVAL_IS_STRING(v));
	JSString* ret = JS_ValueToString(cx, v);
	if (!ret)
		FAIL("Argument must be convertible to a string");
	jschar* ch = JS_GetStringChars(ret);
	out = std::wstring(ch, ch + JS_GetStringLength(ret));
	return true;
}

template<> bool ScriptInterface::FromJSVal<std::string>(JSContext* cx, jsval v, std::string& out)
{
	WARN_IF_NOT(JSVAL_IS_STRING(v));
	JSString* ret = JS_ValueToString(cx, v);
	if (!ret)
		FAIL("Argument must be convertible to a string");
	char* ch = JS_GetStringBytes(ret);
	out = std::string(ch, ch + JS_GetStringLength(ret));
	// TODO: if JS_GetStringBytes fails it'll return a zero-length string
	// and we'll overflow its bounds by using JS_GetStringLength - should
	// use one of the new SpiderMonkey 1.8 functions instead
	return true;
}

/*
template<typename T> bool ScriptInterface::FromJSVal<std::vector<T> >(JSContext* cx, jsval v, std::vector<T>& out)
{
	JSObject* obj;
	if (!JS_ValueToObject(cx, v, &obj) || obj == NULL || !JS_IsArrayObject(cx, obj))
		FAIL("Argument must be an array");
	jsuint length;
	if (!JS_GetArrayLength(cx, obj, &length))
		FAIL("Failed to get array length");
	out.reserve(length);
	for (jsuint i = 0; i < length; ++i)
	{
		jsval el;
		if (!JS_GetElement(cx, obj, i, &el))
			FAIL("Failed to read array element");
		T el2;
		if (!FromJSVal<T>::Convert(cx, el, el2))
			return false;
		out.push_back(el2);
	}
	return true;
}
*/

////////////////////////////////////////////////////////////////
// Primitive types:

template<> jsval ScriptInterface::ToJSVal<bool>(JSContext* UNUSED(cx), const bool& val)
{
	return val ? JSVAL_TRUE : JSVAL_FALSE;
}

template<> jsval ScriptInterface::ToJSVal<float>(JSContext* cx, const float& val)
{
	jsval rval = JSVAL_VOID;
	JS_NewNumberValue(cx, val, &rval); // ignore return value
	return rval;
}

template<> jsval ScriptInterface::ToJSVal<double>(JSContext* cx, const double& val)
{
	jsval rval = JSVAL_VOID;
	JS_NewNumberValue(cx, val, &rval); // ignore return value
	return rval;
}

template<> jsval ScriptInterface::ToJSVal<i32>(JSContext* cx, const i32& val)
{
	if (INT_FITS_IN_JSVAL(val))
		return INT_TO_JSVAL(val);
	jsval rval = JSVAL_VOID;
	JS_NewNumberValue(cx, val, &rval); // ignore return value
	return rval;
}

template<> jsval ScriptInterface::ToJSVal<u32>(JSContext* cx, const u32& val)
{
	if (val <= JSVAL_INT_MAX)
		return INT_TO_JSVAL(val);
	jsval rval = JSVAL_VOID;
	JS_NewNumberValue(cx, val, &rval); // ignore return value
	return rval;
}

// NOTE: we can't define a jsval specialisation, because that conflicts with integer types
template<> jsval ScriptInterface::ToJSVal<CScriptVal>(JSContext* UNUSED(cx), const CScriptVal& val)
{
	return val.get();
}

template<> jsval ScriptInterface::ToJSVal<CScriptValRooted>(JSContext* UNUSED(cx), const CScriptValRooted& val)
{
	return val.get();
}

template<> jsval ScriptInterface::ToJSVal<std::wstring>(JSContext* cx, const std::wstring& val)
{
	utf16string utf16(val.begin(), val.end());
	JSString* str = JS_NewUCStringCopyN(cx, reinterpret_cast<const jschar*> (utf16.c_str()), utf16.length());
	if (str)
		return STRING_TO_JSVAL(str);
	return JSVAL_VOID;
}

template<> jsval ScriptInterface::ToJSVal<std::string>(JSContext* cx, const std::string& val)
{
	JSString* str = JS_NewStringCopyN(cx, val.c_str(), val.length());
	if (str)
		return STRING_TO_JSVAL(str);
	return JSVAL_VOID;
}

template<> jsval ScriptInterface::ToJSVal<const char*>(JSContext* cx, const char* const& val)
{
	JSString* str = JS_NewStringCopyZ(cx, val);
	if (str)
		return STRING_TO_JSVAL(str);
	return JSVAL_VOID;
}

////////////////////////////////////////////////////////////////
// Compound types:

template<typename T> static jsval ToJSVal_vector(JSContext* cx, const std::vector<T>& val)
{
	JSObject* obj = JS_NewArrayObject(cx, 0, NULL);
	if (!obj)
		return JSVAL_VOID;
	JS_AddRoot(cx, &obj);
	for (size_t i = 0; i < val.size(); ++i)
	{
		jsval el = ScriptInterface::ToJSVal<T>(cx, val[i]);
		JS_SetElement(cx, obj, (jsint)i, &el);
	}
	JS_RemoveRoot(cx, &obj);
	return OBJECT_TO_JSVAL(obj);
}

template<> jsval ScriptInterface::ToJSVal<std::vector<int> >(JSContext* cx, const std::vector<int>& val)
{
	return ToJSVal_vector(cx, val);
}

template<> jsval ScriptInterface::ToJSVal<std::vector<u32> >(JSContext* cx, const std::vector<u32>& val)
{
	return ToJSVal_vector(cx, val);
}

template<> jsval ScriptInterface::ToJSVal<std::vector<std::wstring> >(JSContext* cx, const std::vector<std::wstring>& val)
{
	return ToJSVal_vector(cx, val);
}
