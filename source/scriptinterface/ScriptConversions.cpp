/* Copyright (C) 2019 Wildfire Games.
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

#include "graphics/Entity.h"
#include "maths/Vector2D.h"
#include "ps/utf16string.h"
#include "ps/CStr.h"

#define FAIL(msg) STMT(JS_ReportError(cx, msg); return false)

// Implicit type conversions often hide bugs, so warn about them
#define WARN_IF_NOT(c, v) STMT(if (!(c)) { JS_ReportWarning(cx, "Script value conversion check failed: %s (got type %s)", #c, InformalValueTypeName(v)); })

// TODO: SpiderMonkey: Follow upstream progresses about JS_InformalValueTypeName in the API
// https://bugzilla.mozilla.org/show_bug.cgi?id=1285917
static const char* InformalValueTypeName(const JS::Value& v)
{
	if (v.isObject())
		return "object";
	if (v.isString())
		return "string";
	if (v.isSymbol())
		return "symbol";
	if (v.isNumber())
		return "number";
	if (v.isBoolean())
		return "boolean";
	if (v.isNull())
		return "null";
	if (v.isUndefined())
		return "undefined";
	return "value";
}

template<> bool ScriptInterface::FromJSVal<bool>(JSContext* cx, JS::HandleValue v, bool& out)
{
	JSAutoRequest rq(cx);
	WARN_IF_NOT(v.isBoolean(), v);
	out = JS::ToBoolean(v);
	return true;
}

template<> bool ScriptInterface::FromJSVal<float>(JSContext* cx, JS::HandleValue v, float& out)
{
	JSAutoRequest rq(cx);
	double tmp;
	WARN_IF_NOT(v.isNumber(), v);
	if (!JS::ToNumber(cx, v, &tmp))
		return false;
	out = tmp;
	return true;
}

template<> bool ScriptInterface::FromJSVal<double>(JSContext* cx, JS::HandleValue v, double& out)
{
	JSAutoRequest rq(cx);
	WARN_IF_NOT(v.isNumber(), v);
	if (!JS::ToNumber(cx, v, &out))
		return false;
	return true;
}

template<> bool ScriptInterface::FromJSVal<i32>(JSContext* cx, JS::HandleValue v, i32& out)
{
	JSAutoRequest rq(cx);
	WARN_IF_NOT(v.isNumber(), v);
	if (!JS::ToInt32(cx, v, &out))
		return false;
	return true;
}

template<> bool ScriptInterface::FromJSVal<u32>(JSContext* cx, JS::HandleValue v, u32& out)
{
	JSAutoRequest rq(cx);
	WARN_IF_NOT(v.isNumber(), v);
	if (!JS::ToUint32(cx, v, &out))
		return false;
	return true;
}

template<> bool ScriptInterface::FromJSVal<u16>(JSContext* cx, JS::HandleValue v, u16& out)
{
	JSAutoRequest rq(cx);
	WARN_IF_NOT(v.isNumber(), v);
	if (!JS::ToUint16(cx, v, &out))
		return false;
	return true;
}

template<> bool ScriptInterface::FromJSVal<u8>(JSContext* cx, JS::HandleValue v, u8& out)
{
	JSAutoRequest rq(cx);
	u16 tmp;
	WARN_IF_NOT(v.isNumber(), v);
	if (!JS::ToUint16(cx, v, &tmp))
		return false;
	out = (u8)tmp;
	return true;
}

template<> bool ScriptInterface::FromJSVal<std::wstring>(JSContext* cx, JS::HandleValue v, std::wstring& out)
{
	JSAutoRequest rq(cx);
	WARN_IF_NOT(v.isString() || v.isNumber(), v); // allow implicit number conversions
	JS::RootedString str(cx, JS::ToString(cx, v));
	if (!str)
		FAIL("Argument must be convertible to a string");

	if (JS_StringHasLatin1Chars(str))
	{
		size_t length;
		JS::AutoCheckCannotGC nogc;
		const JS::Latin1Char* ch = JS_GetLatin1StringCharsAndLength(cx, nogc, str, &length);
		if (!ch)
			FAIL("JS_GetLatin1StringCharsAndLength failed");

		out.assign(ch, ch + length);
	}
	else
	{
		size_t length;
		JS::AutoCheckCannotGC nogc;
		const char16_t* ch = JS_GetTwoByteStringCharsAndLength(cx, nogc, str, &length);
		if (!ch)
			FAIL("JS_GetTwoByteStringsCharsAndLength failed"); // out of memory

		out.assign(ch, ch + length);
	}
	return true;
}

template<> bool ScriptInterface::FromJSVal<Path>(JSContext* cx, JS::HandleValue v, Path& out)
{
	std::wstring string;
	if (!FromJSVal(cx, v, string))
		return false;
	out = string;
	return true;
}

template<> bool ScriptInterface::FromJSVal<std::string>(JSContext* cx, JS::HandleValue v, std::string& out)
{
	JSAutoRequest rq(cx);
	WARN_IF_NOT(v.isString() || v.isNumber(), v); // allow implicit number conversions
	JS::RootedString str(cx, JS::ToString(cx, v));
	if (!str)
		FAIL("Argument must be convertible to a string");
	char* ch = JS_EncodeString(cx, str); // chops off high byte of each char16_t
	if (!ch)
		FAIL("JS_EncodeString failed"); // out of memory
	out.assign(ch, ch + JS_GetStringLength(str));
	JS_free(cx, ch);
	return true;
}

template<> bool ScriptInterface::FromJSVal<CStr8>(JSContext* cx, JS::HandleValue v, CStr8& out)
{
	return ScriptInterface::FromJSVal(cx, v, static_cast<std::string&>(out));
}

template<> bool ScriptInterface::FromJSVal<CStrW>(JSContext* cx, JS::HandleValue v, CStrW& out)
{
	return ScriptInterface::FromJSVal(cx, v, static_cast<std::wstring&>(out));
}

template<> bool ScriptInterface::FromJSVal<Entity>(JSContext* cx, JS::HandleValue v, Entity& out)
{
	JSAutoRequest rq(cx);
	if (!v.isObject())
		FAIL("Argument must be an object");

	JS::RootedObject obj(cx, &v.toObject());
	JS::RootedValue templateName(cx);
	JS::RootedValue id(cx);
	JS::RootedValue player(cx);
	JS::RootedValue position(cx);
	JS::RootedValue rotation(cx);

	// TODO: Report type errors
	if (!JS_GetProperty(cx, obj, "player", &player) || !FromJSVal(cx, player, out.playerID))
		FAIL("Failed to read Entity.player property");
	if (!JS_GetProperty(cx, obj, "templateName", &templateName) || !FromJSVal(cx, templateName, out.templateName))
		FAIL("Failed to read Entity.templateName property");
	if (!JS_GetProperty(cx, obj, "id", &id) || !FromJSVal(cx, id, out.entityID))
		FAIL("Failed to read Entity.id property");
	if (!JS_GetProperty(cx, obj, "position", &position) || !FromJSVal(cx, position, out.position))
		FAIL("Failed to read Entity.position property");
	if (!JS_GetProperty(cx, obj, "rotation", &rotation) || !FromJSVal(cx, rotation, out.rotation))
		FAIL("Failed to read Entity.rotation property");

	return true;
}

////////////////////////////////////////////////////////////////
// Primitive types:

template<> void ScriptInterface::ToJSVal<bool>(JSContext* UNUSED(cx), JS::MutableHandleValue ret, const bool& val)
{
	ret.setBoolean(val);
}

template<> void ScriptInterface::ToJSVal<float>(JSContext* UNUSED(cx), JS::MutableHandleValue ret, const float& val)
{
	ret.set(JS::NumberValue(val));
}

template<> void ScriptInterface::ToJSVal<double>(JSContext* UNUSED(cx), JS::MutableHandleValue ret, const double& val)
{
	ret.set(JS::NumberValue(val));
}

template<> void ScriptInterface::ToJSVal<i32>(JSContext* UNUSED(cx), JS::MutableHandleValue ret, const i32& val)
{
	ret.set(JS::NumberValue(val));
}

template<> void ScriptInterface::ToJSVal<u16>(JSContext* UNUSED(cx), JS::MutableHandleValue ret, const u16& val)
{
	ret.set(JS::NumberValue(val));
}

template<> void ScriptInterface::ToJSVal<u8>(JSContext* UNUSED(cx), JS::MutableHandleValue ret, const u8& val)
{
	ret.set(JS::NumberValue(val));
}

template<> void ScriptInterface::ToJSVal<u32>(JSContext* UNUSED(cx), JS::MutableHandleValue ret, const u32& val)
{
	ret.set(JS::NumberValue(val));
}

template<> void ScriptInterface::ToJSVal<std::wstring>(JSContext* cx, JS::MutableHandleValue ret, const std::wstring& val)
{
	JSAutoRequest rq(cx);
	utf16string utf16(val.begin(), val.end());
	JS::RootedString str(cx, JS_NewUCStringCopyN(cx, reinterpret_cast<const char16_t*> (utf16.c_str()), utf16.length()));
	if (str)
		ret.setString(str);
	else
		ret.setUndefined();
}

template<> void ScriptInterface::ToJSVal<Path>(JSContext* cx, JS::MutableHandleValue ret, const Path& val)
{
	ToJSVal(cx, ret, val.string());
}

template<> void ScriptInterface::ToJSVal<std::string>(JSContext* cx, JS::MutableHandleValue ret, const std::string& val)
{
	JSAutoRequest rq(cx);
	JS::RootedString str(cx, JS_NewStringCopyN(cx, val.c_str(), val.length()));
	if (str)
		ret.setString(str);
	else
		ret.setUndefined();
}

template<> void ScriptInterface::ToJSVal<const wchar_t*>(JSContext* cx, JS::MutableHandleValue ret, const wchar_t* const& val)
{
	ToJSVal(cx, ret, std::wstring(val));
}

template<> void ScriptInterface::ToJSVal<const char*>(JSContext* cx, JS::MutableHandleValue ret, const char* const& val)
{
	JSAutoRequest rq(cx);
	JS::RootedString str(cx, JS_NewStringCopyZ(cx, val));
	if (str)
		ret.setString(str);
	else
		ret.setUndefined();
}

#define TOJSVAL_CHAR(N) \
template<> void ScriptInterface::ToJSVal<wchar_t[N]>(JSContext* cx, JS::MutableHandleValue ret, const wchar_t (&val)[N]) \
{ \
	ToJSVal(cx, ret, static_cast<const wchar_t*>(val)); \
} \
template<> void ScriptInterface::ToJSVal<char[N]>(JSContext* cx, JS::MutableHandleValue ret, const char (&val)[N]) \
{ \
	ToJSVal(cx, ret, static_cast<const char*>(val)); \
}

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

template<> void ScriptInterface::ToJSVal<CStrW>(JSContext* cx, JS::MutableHandleValue ret, const CStrW& val)
{
	ToJSVal(cx, ret, static_cast<const std::wstring&>(val));
}

template<> void ScriptInterface::ToJSVal<CStr8>(JSContext* cx, JS::MutableHandleValue ret, const CStr8& val)
{
	ToJSVal(cx, ret, static_cast<const std::string&>(val));
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
JSVAL_VECTOR(std::vector<CStr8>)
JSVAL_VECTOR(std::vector<std::string>)


class IComponent;
template<> void ScriptInterface::ToJSVal<std::vector<IComponent*> >(JSContext* cx, JS::MutableHandleValue ret, const std::vector<IComponent*>& val)
{
	ToJSVal_vector(cx, ret, val);
}

template<> bool ScriptInterface::FromJSVal<std::vector<Entity> >(JSContext* cx, JS::HandleValue v, std::vector<Entity>& out)
{
	return FromJSVal_vector(cx, v, out);
}

template<> void ScriptInterface::ToJSVal<CVector2D>(JSContext* cx, JS::MutableHandleValue ret, const CVector2D& val)
{
	std::vector<float> vec = {val.X, val.Y};
	ToJSVal_vector(cx, ret, vec);
}

template<> bool ScriptInterface::FromJSVal<CVector2D>(JSContext* cx, JS::HandleValue v, CVector2D& out)
{
	std::vector<float> vec;

	if (!FromJSVal_vector(cx, v, vec))
		return false;

	if (vec.size() != 2)
		return false;

	out.X = vec[0];
	out.Y = vec[1];

	return true;
}

#undef FAIL
#undef WARN_IF_NOT
