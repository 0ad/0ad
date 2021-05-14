/* Copyright (C) 2021 Wildfire Games.
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

#include "ScriptConversions.h"
#include "ScriptExceptions.h"
#include "ScriptExtraHeaders.h"

#include "graphics/Entity.h"
#include "lib/file/vfs/vfs_path.h"
#include "maths/Vector2D.h"
#include "ps/utf16string.h"
#include "ps/CLogger.h"
#include "ps/CStr.h"

// Catch the raised exception right away to ensure the stack trace gets printed.
#define FAIL(msg) STMT(ScriptException::Raise(rq, msg); ScriptException::CatchPending(rq); return false)

// Implicit type conversions often hide bugs, so fail.
#define FAIL_IF_NOT(c, v) STMT(if (!(c)) { \
	ScriptException::Raise(rq, "Script value conversion check failed: %s (got type %s)", #c, JS::InformalValueTypeName(v)); \
	ScriptException::CatchPending(rq); \
	return false; \
})

template<> bool Script::FromJSVal<bool>(const ScriptRequest& rq, JS::HandleValue v, bool& out)
{
	FAIL_IF_NOT(v.isBoolean(), v);
	out = JS::ToBoolean(v);
	return true;
}

template<> bool Script::FromJSVal<float>(const ScriptRequest& rq, JS::HandleValue v, float& out)
{
	double tmp;
	FAIL_IF_NOT(v.isNumber(), v);
	if (!JS::ToNumber(rq.cx, v, &tmp))
		return false;
	out = tmp;
	return true;
}

template<> bool Script::FromJSVal<double>(const ScriptRequest& rq,  JS::HandleValue v, double& out)
{
	FAIL_IF_NOT(v.isNumber(), v);
	if (!JS::ToNumber(rq.cx, v, &out))
		return false;
	return true;
}

template<> bool Script::FromJSVal<i32>(const ScriptRequest& rq,  JS::HandleValue v, i32& out)
{
	FAIL_IF_NOT(v.isNumber(), v);
	if (!JS::ToInt32(rq.cx, v, &out))
		return false;
	return true;
}

template<> bool Script::FromJSVal<u32>(const ScriptRequest& rq,  JS::HandleValue v, u32& out)
{
	FAIL_IF_NOT(v.isNumber(), v);
	if (!JS::ToUint32(rq.cx, v, &out))
		return false;
	return true;
}

template<> bool Script::FromJSVal<u16>(const ScriptRequest& rq,  JS::HandleValue v, u16& out)
{
	FAIL_IF_NOT(v.isNumber(), v);
	if (!JS::ToUint16(rq.cx, v, &out))
		return false;
	return true;
}

template<> bool Script::FromJSVal<u8>(const ScriptRequest& rq,  JS::HandleValue v, u8& out)
{
	u16 tmp;
	FAIL_IF_NOT(v.isNumber(), v);
	if (!JS::ToUint16(rq.cx, v, &tmp))
		return false;
	out = (u8)tmp;
	return true;
}

template<> bool Script::FromJSVal<std::wstring>(const ScriptRequest& rq,  JS::HandleValue v, std::wstring& out)
{
	FAIL_IF_NOT(v.isString() || v.isNumber() || v.isBoolean(), v); // allow implicit boolean/number conversions
	JS::RootedString str(rq.cx, JS::ToString(rq.cx, v));
	if (!str)
		FAIL("Argument must be convertible to a string");

	if (JS_StringHasLatin1Chars(str))
	{
		size_t length;
		JS::AutoCheckCannotGC nogc;
		const JS::Latin1Char* ch = JS_GetLatin1StringCharsAndLength(rq.cx, nogc, str, &length);
		if (!ch)
			FAIL("JS_GetLatin1StringCharsAndLength failed");

		out.assign(ch, ch + length);
	}
	else
	{
		size_t length;
		JS::AutoCheckCannotGC nogc;
		const char16_t* ch = JS_GetTwoByteStringCharsAndLength(rq.cx, nogc, str, &length);
		if (!ch)
			FAIL("JS_GetTwoByteStringsCharsAndLength failed"); // out of memory

		out.assign(ch, ch + length);
	}
	return true;
}

template<> bool Script::FromJSVal<Path>(const ScriptRequest& rq,  JS::HandleValue v, Path& out)
{
	std::wstring string;
	if (!FromJSVal(rq, v, string))
		return false;
	out = string;
	return true;
}

template<> bool Script::FromJSVal<std::string>(const ScriptRequest& rq,  JS::HandleValue v, std::string& out)
{
	std::wstring wideout;
	if (!FromJSVal(rq, v, wideout))
		return false;
	out = CStrW(wideout).ToUTF8();
	return true;
}

template<> bool Script::FromJSVal<CStr8>(const ScriptRequest& rq,  JS::HandleValue v, CStr8& out)
{
	return Script::FromJSVal(rq, v, static_cast<std::string&>(out));
}

template<> bool Script::FromJSVal<CStrW>(const ScriptRequest& rq,  JS::HandleValue v, CStrW& out)
{
	return Script::FromJSVal(rq, v, static_cast<std::wstring&>(out));
}

template<> bool Script::FromJSVal<Entity>(const ScriptRequest& rq,  JS::HandleValue v, Entity& out)
{
	if (!v.isObject())
		FAIL("Argument must be an object");

	JS::RootedObject obj(rq.cx, &v.toObject());
	JS::RootedValue templateName(rq.cx);
	JS::RootedValue id(rq.cx);
	JS::RootedValue player(rq.cx);
	JS::RootedValue position(rq.cx);
	JS::RootedValue rotation(rq.cx);

	// TODO: Report type errors
	if (!JS_GetProperty(rq.cx, obj, "player", &player) || !FromJSVal(rq, player, out.playerID))
		FAIL("Failed to read Entity.player property");
	if (!JS_GetProperty(rq.cx, obj, "templateName", &templateName) || !FromJSVal(rq, templateName, out.templateName))
		FAIL("Failed to read Entity.templateName property");
	if (!JS_GetProperty(rq.cx, obj, "id", &id) || !FromJSVal(rq, id, out.entityID))
		FAIL("Failed to read Entity.id property");
	if (!JS_GetProperty(rq.cx, obj, "position", &position) || !FromJSVal(rq, position, out.position))
		FAIL("Failed to read Entity.position property");
	if (!JS_GetProperty(rq.cx, obj, "rotation", &rotation) || !FromJSVal(rq, rotation, out.rotation))
		FAIL("Failed to read Entity.rotation property");

	return true;
}

////////////////////////////////////////////////////////////////
// Primitive types:

template<> void Script::ToJSVal<bool>(const ScriptRequest& UNUSED(rq), JS::MutableHandleValue ret, const bool& val)
{
	ret.setBoolean(val);
}

template<> void Script::ToJSVal<float>(const ScriptRequest& UNUSED(rq), JS::MutableHandleValue ret, const float& val)
{
	ret.set(JS::NumberValue(val));
}

template<> void Script::ToJSVal<double>(const ScriptRequest& UNUSED(rq), JS::MutableHandleValue ret, const double& val)
{
	ret.set(JS::NumberValue(val));
}

template<> void Script::ToJSVal<i32>(const ScriptRequest& UNUSED(rq), JS::MutableHandleValue ret, const i32& val)
{
	ret.set(JS::NumberValue(val));
}

template<> void Script::ToJSVal<u16>(const ScriptRequest& UNUSED(rq), JS::MutableHandleValue ret, const u16& val)
{
	ret.set(JS::NumberValue(val));
}

template<> void Script::ToJSVal<u8>(const ScriptRequest& UNUSED(rq), JS::MutableHandleValue ret, const u8& val)
{
	ret.set(JS::NumberValue(val));
}

template<> void Script::ToJSVal<u32>(const ScriptRequest& UNUSED(rq), JS::MutableHandleValue ret, const u32& val)
{
	ret.set(JS::NumberValue(val));
}

template<> void Script::ToJSVal<std::wstring>(const ScriptRequest& rq,  JS::MutableHandleValue ret, const std::wstring& val)
{
	utf16string utf16(val.begin(), val.end());
	JS::RootedString str(rq.cx, JS_NewUCStringCopyN(rq.cx, reinterpret_cast<const char16_t*> (utf16.c_str()), utf16.length()));
	if (str)
		ret.setString(str);
	else
		ret.setUndefined();
}

template<> void Script::ToJSVal<Path>(const ScriptRequest& rq,  JS::MutableHandleValue ret, const Path& val)
{
	ToJSVal(rq, ret, val.string());
}

template<> void Script::ToJSVal<std::string>(const ScriptRequest& rq,  JS::MutableHandleValue ret, const std::string& val)
{
	ToJSVal(rq, ret, static_cast<const std::wstring>(CStr(val).FromUTF8()));
}

template<> void Script::ToJSVal<const wchar_t*>(const ScriptRequest& rq,  JS::MutableHandleValue ret, const wchar_t* const& val)
{
	ToJSVal(rq, ret, std::wstring(val));
}

template<> void Script::ToJSVal<const char*>(const ScriptRequest& rq,  JS::MutableHandleValue ret, const char* const& val)
{
	JS::RootedString str(rq.cx, JS_NewStringCopyZ(rq.cx, val));
	if (str)
		ret.setString(str);
	else
		ret.setUndefined();
}

#define TOJSVAL_CHAR(N) \
template<> void Script::ToJSVal<wchar_t[N]>(const ScriptRequest& rq,  JS::MutableHandleValue ret, const wchar_t (&val)[N]) \
{ \
	ToJSVal(rq, ret, static_cast<const wchar_t*>(val)); \
} \
template<> void Script::ToJSVal<char[N]>(const ScriptRequest& rq,  JS::MutableHandleValue ret, const char (&val)[N]) \
{ \
	ToJSVal(rq, ret, static_cast<const char*>(val)); \
}

TOJSVAL_CHAR(3)
TOJSVAL_CHAR(5)
TOJSVAL_CHAR(6)
TOJSVAL_CHAR(7)
TOJSVAL_CHAR(8)
TOJSVAL_CHAR(9)
TOJSVAL_CHAR(10)
TOJSVAL_CHAR(11)
TOJSVAL_CHAR(12)
TOJSVAL_CHAR(13)
TOJSVAL_CHAR(14)
TOJSVAL_CHAR(15)
TOJSVAL_CHAR(16)
TOJSVAL_CHAR(17)
TOJSVAL_CHAR(18)
TOJSVAL_CHAR(19)
TOJSVAL_CHAR(20)
TOJSVAL_CHAR(24)
TOJSVAL_CHAR(29)
TOJSVAL_CHAR(33)
TOJSVAL_CHAR(35)
TOJSVAL_CHAR(256)
#undef TOJSVAL_CHAR

template<> void Script::ToJSVal<CStrW>(const ScriptRequest& rq,  JS::MutableHandleValue ret, const CStrW& val)
{
	ToJSVal(rq, ret, static_cast<const std::wstring&>(val));
}

template<> void Script::ToJSVal<CStr8>(const ScriptRequest& rq,  JS::MutableHandleValue ret, const CStr8& val)
{
	ToJSVal(rq, ret, static_cast<const std::string&>(val));
}

////////////////////////////////////////////////////////////////
// Compound types
// Instantiate various vector types:

JSVAL_VECTOR(int)
JSVAL_VECTOR(u32)
JSVAL_VECTOR(u16)
JSVAL_VECTOR(std::string)
JSVAL_VECTOR(std::wstring)
JSVAL_VECTOR(std::vector<std::wstring>)
JSVAL_VECTOR(CStr8)
JSVAL_VECTOR(CStrW)
JSVAL_VECTOR(std::vector<CStr8>)
JSVAL_VECTOR(std::vector<std::string>)


class IComponent;
template<> void Script::ToJSVal<std::vector<IComponent*>>(const ScriptRequest& rq,  JS::MutableHandleValue ret, const std::vector<IComponent*>& val)
{
	ToJSVal_vector(rq, ret, val);
}

template<> bool Script::FromJSVal<std::vector<Entity>>(const ScriptRequest& rq,  JS::HandleValue v, std::vector<Entity>& out)
{
	return FromJSVal_vector(rq, v, out);
}

#undef FAIL
#undef FAIL_IF_NOT
