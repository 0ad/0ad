/* Copyright (C) 2011 Wildfire Games.
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

#include "graphics/Entity.h"
#include "ps/utf16string.h"
#include "ps/CLogger.h"
#include "ps/CStr.h"
#include "scriptinterface/ScriptExtraHeaders.h" // for typed arrays

#define FAIL(msg) STMT(JS_ReportError(cx, msg); return false)

// Implicit type conversions often hide bugs, so warn about them
#define WARN_IF_NOT(c, v) STMT(if (!(c)) { JS_ReportWarning(cx, "Script value conversion check failed: %s (got type %s)", #c, JS_GetTypeName(cx, JS_TypeOfValue(cx, v))); })

template<> bool ScriptInterface::FromJSVal<bool>(JSContext* cx, jsval v, bool& out)
{
	JSBool ret;
	WARN_IF_NOT(JSVAL_IS_BOOLEAN(v), v);
	if (!JS_ValueToBoolean(cx, v, &ret))
		return false;
	out = (ret ? true : false);
	return true;
}

template<> bool ScriptInterface::FromJSVal<float>(JSContext* cx, jsval v, float& out)
{
	jsdouble ret;
	WARN_IF_NOT(JSVAL_IS_NUMBER(v), v);
	if (!JS_ValueToNumber(cx, v, &ret))
		return false;
	out = ret;
	return true;
}

template<> bool ScriptInterface::FromJSVal<double>(JSContext* cx, jsval v, double& out)
{
	jsdouble ret;
	WARN_IF_NOT(JSVAL_IS_NUMBER(v), v);
	if (!JS_ValueToNumber(cx, v, &ret))
		return false;
	out = ret;
	return true;
}

template<> bool ScriptInterface::FromJSVal<i32>(JSContext* cx, jsval v, i32& out)
{
	int32 ret;
	WARN_IF_NOT(JSVAL_IS_NUMBER(v), v);
	if (!JS_ValueToECMAInt32(cx, v, &ret))
		return false;
	out = ret;
	return true;
}

template<> bool ScriptInterface::FromJSVal<u32>(JSContext* cx, jsval v, u32& out)
{
	uint32 ret;
	WARN_IF_NOT(JSVAL_IS_NUMBER(v), v);
	if (!JS_ValueToECMAUint32(cx, v, &ret))
		return false;
	out = ret;
	return true;
}

template<> bool ScriptInterface::FromJSVal<u16>(JSContext* cx, jsval v, u16& out)
{
	uint16 ret;
	WARN_IF_NOT(JSVAL_IS_NUMBER(v), v);
	if (!JS_ValueToUint16(cx, v, &ret))
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
	WARN_IF_NOT(JSVAL_IS_STRING(v) || JSVAL_IS_NUMBER(v), v); // allow implicit number conversions
	JSString* ret = JS_ValueToString(cx, v);
	if (!ret)
		FAIL("Argument must be convertible to a string");
	size_t length;
	const jschar* ch = JS_GetStringCharsAndLength(cx, ret, &length);
	if (!ch)
		FAIL("JS_GetStringsCharsAndLength failed"); // out of memory
	out = std::wstring(ch, ch + length);
	return true;
}

template<> bool ScriptInterface::FromJSVal<Path>(JSContext* cx, jsval v, Path& out)
{
	std::wstring string;
	if (!FromJSVal(cx, v, string))
		return false;
	out = string;
	return true;
}

template<> bool ScriptInterface::FromJSVal<std::string>(JSContext* cx, jsval v, std::string& out)
{
	WARN_IF_NOT(JSVAL_IS_STRING(v) || JSVAL_IS_NUMBER(v), v); // allow implicit number conversions
	JSString* ret = JS_ValueToString(cx, v);
	if (!ret)
		FAIL("Argument must be convertible to a string");
	char* ch = JS_EncodeString(cx, ret); // chops off high byte of each jschar
	if (!ch)
		FAIL("JS_EncodeString failed"); // out of memory
	out = std::string(ch, ch + JS_GetStringLength(ret));
	JS_free(cx, ch);
	return true;
}

template<> bool ScriptInterface::FromJSVal<Entity>(JSContext* cx, jsval v, Entity& out)
{
	JSObject* obj;
	if (!JS_ValueToObject(cx, v, &obj) || obj == NULL)
		FAIL("Argument must be an object");

	jsval name, id, player, x, z, orient;

	if(!JS_GetProperty(cx, obj, "player", &player) || !FromJSVal(cx, player, out.playerID))
		FAIL("Failed to read Entity.player property");
	if (!JS_GetProperty(cx, obj, "name", &name) || !FromJSVal(cx, name, out.templateName))
		FAIL("Failed to read Entity.name property");
	if (!JS_GetProperty(cx, obj, "id", &id) || !FromJSVal(cx, id, out.entityID))
		FAIL("Failed to read Entity.id property");
	if (!JS_GetProperty(cx, obj, "x", &x) || !FromJSVal(cx, x, out.positionX))
		FAIL("Failed to read Entity.x property");
	if (!JS_GetProperty(cx, obj, "z", &z) || !FromJSVal(cx, z, out.positionZ))
		FAIL("Failed to read Entity.z property");
	if (!JS_GetProperty(cx, obj, "orientation", &orient) || !FromJSVal(cx, orient, out.orientationY))
		FAIL("Failed to read Entity.orientation property");

	return true;
}

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

template<> jsval ScriptInterface::ToJSVal<i32>(JSContext* UNUSED(cx), const i32& val)
{
	cassert(JSVAL_INT_BITS == 32);
	return INT_TO_JSVAL(val);
}

template<> jsval ScriptInterface::ToJSVal<u16>(JSContext* UNUSED(cx), const u16& val)
{
	return INT_TO_JSVAL(val);
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

template<> jsval ScriptInterface::ToJSVal<Path>(JSContext* cx, const Path& val)
{
	return ToJSVal(cx, val.string());
}

template<> jsval ScriptInterface::ToJSVal<std::string>(JSContext* cx, const std::string& val)
{
	JSString* str = JS_NewStringCopyN(cx, val.c_str(), val.length());
	if (str)
		return STRING_TO_JSVAL(str);
	return JSVAL_VOID;
}

template<> jsval ScriptInterface::ToJSVal<const wchar_t*>(JSContext* cx, const wchar_t* const& val)
{
	return ToJSVal(cx, std::wstring(val));
}

template<> jsval ScriptInterface::ToJSVal<const char*>(JSContext* cx, const char* const& val)
{
	JSString* str = JS_NewStringCopyZ(cx, val);
	if (str)
		return STRING_TO_JSVAL(str);
	return JSVAL_VOID;
}

template<> jsval ScriptInterface::ToJSVal<CStrW>(JSContext* cx, const CStrW& val)
{
	return ToJSVal(cx, static_cast<const std::wstring&>(val));
}

template<> jsval ScriptInterface::ToJSVal<CStr8>(JSContext* cx, const CStr8& val)
{
	return ToJSVal(cx, static_cast<const std::string&>(val));
}

////////////////////////////////////////////////////////////////
// Compound types:

template<typename T> static jsval ToJSVal_vector(JSContext* cx, const std::vector<T>& val)
{
	JSObject* obj = JS_NewArrayObject(cx, val.size(), NULL);
	if (!obj)
		return JSVAL_VOID;
	for (size_t i = 0; i < val.size(); ++i)
	{
		jsval el = ScriptInterface::ToJSVal<T>(cx, val[i]);
		JS_SetElement(cx, obj, (jsint)i, &el);
	}
	return OBJECT_TO_JSVAL(obj);
}

template<typename T> static bool FromJSVal_vector(JSContext* cx, jsval v, std::vector<T>& out)
{
	JSObject* obj;
	if (!JS_ValueToObject(cx, v, &obj) || obj == NULL || !(JS_IsArrayObject(cx, obj) || js_IsTypedArray(obj)))
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
		if (!ScriptInterface::FromJSVal<T>(cx, el, el2))
			return false;
		out.push_back(el2);
	}
	return true;
}

// Instantiate various vector types:

#define VECTOR(T) \
	template<> jsval ScriptInterface::ToJSVal<std::vector<T> >(JSContext* cx, const std::vector<T>& val) \
	{ \
		return ToJSVal_vector(cx, val); \
	} \
	template<> bool ScriptInterface::FromJSVal<std::vector<T> >(JSContext* cx, jsval v, std::vector<T>& out) \
	{ \
		return FromJSVal_vector(cx, v, out); \
	}

VECTOR(int)
VECTOR(u32)
VECTOR(u16)
VECTOR(std::string)
VECTOR(std::wstring)
VECTOR(CScriptValRooted)


class IComponent;
template<> jsval ScriptInterface::ToJSVal<std::vector<IComponent*> >(JSContext* cx, const std::vector<IComponent*>& val)
{
	return ToJSVal_vector(cx, val);
}

template<> bool ScriptInterface::FromJSVal<std::vector<Entity> >(JSContext* cx, jsval v, std::vector<Entity>& out)
{
	return FromJSVal_vector(cx, v, out);
}
