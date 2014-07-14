/* Copyright (C) 2013 Wildfire Games.
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

template<> bool ScriptInterface::FromJSVal<long>(JSContext* cx, JS::HandleValue v, long& out)
{
	i64 tmp;
	bool ok = JS::ToInt64(cx, v, &tmp);
	out = (long)tmp;
	return ok;
}

template<> bool ScriptInterface::FromJSVal<unsigned long>(JSContext* cx, JS::HandleValue v, unsigned long& out)
{
	u64 tmp;
	bool ok = JS::ToUint64(cx, v, &tmp);
	out = (unsigned long)tmp;
	return ok;
}

// see comment below (where the same preprocessor condition is used)
#if MSC_VERSION && ARCH_AMD64

template<> bool ScriptInterface::FromJSVal<size_t>(JSContext* cx, JS::HandleValue v, size_t& out)
{
	int tmp;
	if(!FromJSVal<int>(cx, v, tmp))
		return false;
	if(temp < 0)
		return false;
	out = (size_t)tmp;
	return true;
}

template<> bool ScriptInterface::FromJSVal<ssize_t>(JSContext* cx, JS::HandleValue v, ssize_t& out)
{
	int tmp;
	if(!FromJSVal<int>(cx, v, tmp))
		return false;
	if(tmp < 0)
		return false;
	out = (ssize_t)tmp;
	return true;
}

#endif

template<> bool ScriptInterface::FromJSVal<CScriptVal>(JSContext* UNUSED(cx), JS::HandleValue v, CScriptVal& out)
{
	out = v.get();
	return true;
}

template<> bool ScriptInterface::FromJSVal<CScriptValRooted>(JSContext* cx, JS::HandleValue v, CScriptValRooted& out)
{
	out = CScriptValRooted(cx, v);
	return true;
}

template<> bool ScriptInterface::FromJSVal<std::wstring>(JSContext* cx, JS::HandleValue v, std::wstring& out)
{
	JSAutoRequest rq(cx);
	WARN_IF_NOT(JSVAL_IS_STRING(v) || JSVAL_IS_NUMBER(v), v); // allow implicit number conversions
	JSString* str = JS_ValueToString(cx, v);
	if (!str)
		FAIL("Argument must be convertible to a string");
	size_t length;
	const jschar* ch = JS_GetStringCharsAndLength(cx, str, &length);
	if (!ch)
		FAIL("JS_GetStringsCharsAndLength failed"); // out of memory
	out = std::wstring(ch, ch + length);
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
	JSString* str = JS_ValueToString(cx, v);
	if (!str)
		FAIL("Argument must be convertible to a string");
	char* ch = JS_EncodeString(cx, str); // chops off high byte of each jschar
	if (!ch)
		FAIL("JS_EncodeString failed"); // out of memory
	out = std::string(ch, ch + JS_GetStringLength(str));
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
	if(!JS_GetProperty(cx, obj, "player", player.address()) || !FromJSVal(cx, player, out.playerID))
		FAIL("Failed to read Entity.player property");
	if (!JS_GetProperty(cx, obj, "templateName", templateName.address()) || !FromJSVal(cx, templateName, out.templateName))
		FAIL("Failed to read Entity.templateName property");
	if (!JS_GetProperty(cx, obj, "id", id.address()) || !FromJSVal(cx, id, out.entityID))
		FAIL("Failed to read Entity.id property");
	if (!JS_GetProperty(cx, obj, "position", position.address()) || !FromJSVal(cx, position, out.position))
		FAIL("Failed to read Entity.position property");
	if (!JS_GetProperty(cx, obj, "rotation", rotation.address()) || !FromJSVal(cx, rotation, out.rotation))
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

template<> void ScriptInterface::ToJSVal<long>(JSContext* UNUSED(cx), JS::MutableHandleValue ret, const long& val)
{
	ret.set(JS::NumberValue((int)val));
}

template<> void ScriptInterface::ToJSVal<unsigned long>(JSContext* UNUSED(cx), JS::MutableHandleValue ret, const unsigned long& val)
{
	ret.set(JS::NumberValue((int)val));
}

// (s)size_t are considered to be identical to (unsigned) int by GCC and 
// their specializations would cause conflicts there. On x86_64 GCC, s/size_t 
// is equivalent to (unsigned) long, but the same solution applies; use the 
// long and unsigned long specializations instead of s/size_t. 
// for some reason, x64 MSC treats size_t as distinct from unsigned long:
#if MSC_VERSION && ARCH_AMD64

template<> void ScriptInterface::ToJSVal<size_t>(JSContext* UNUSED(cx), JS::MutableHandleValue ret, const size_t& val)
{
	ret.set(JS::NumberValue((int)val));
}

template<> void ScriptInterface::ToJSVal<ssize_t>(JSContext* UNUSED(cx), JS::MutableHandleValue ret, const ssize_t& val)
{
	ret.set(JS::NumberValue((int)val));
}

#endif

template<> void ScriptInterface::ToJSVal<CScriptVal>(JSContext* UNUSED(cx), JS::MutableHandleValue ret, const CScriptVal& val)
{
	ret.set(val.get());
}

template<> void ScriptInterface::ToJSVal<CScriptValRooted>(JSContext* UNUSED(cx), JS::MutableHandleValue ret, const CScriptValRooted& val)
{
	ret.set(val.get());
}

template<> void ScriptInterface::ToJSVal<std::wstring>(JSContext* cx, JS::MutableHandleValue ret, const std::wstring& val)
{
	JSAutoRequest rq(cx);
	utf16string utf16(val.begin(), val.end());
	JSString* str = JS_NewUCStringCopyN(cx, reinterpret_cast<const jschar*> (utf16.c_str()), utf16.length());
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
	JSString* str = JS_NewStringCopyN(cx, val.c_str(), val.length());
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
	JSString* str = JS_NewStringCopyZ(cx, val);
	if (str)
		ret.setString(str);
	else
		ret.setUndefined();
}

template<> void ScriptInterface::ToJSVal<CStrW>(JSContext* cx, JS::MutableHandleValue ret, const CStrW& val)
{
	ToJSVal(cx, ret, static_cast<const std::wstring&>(val));
}

template<> void ScriptInterface::ToJSVal<CStr8>(JSContext* cx, JS::MutableHandleValue ret, const CStr8& val)
{
	ToJSVal(cx, ret, static_cast<const std::string&>(val));
}

////////////////////////////////////////////////////////////////
// Compound types:

template<typename T> static void ToJSVal_vector(JSContext* cx, JS::MutableHandleValue ret, const std::vector<T>& val)
{
	JSAutoRequest rq(cx);
	JSObject* obj = JS_NewArrayObject(cx, val.size(), NULL);
	if (!obj)
	{
		ret.setUndefined();
		return;
	}
	for (u32 i = 0; i < val.size(); ++i)
	{
		JS::RootedValue el(cx);
		ScriptInterface::ToJSVal<T>(cx, &el, val[i]);
		JS_SetElement(cx, obj, i, el.address());
	}
	ret.setObject(*obj);
}

template<typename T> static bool FromJSVal_vector(JSContext* cx, JS::HandleValue v, std::vector<T>& out)
{
	JSAutoRequest rq(cx);
	JSObject* obj;
	if (!v.isObject())
		FAIL("Argument must be an array");
	obj = &v.toObject();
	if (!(JS_IsArrayObject(cx, obj) || JS_IsTypedArrayObject(obj)))
		FAIL("Argument must be an array");
	
	u32 length;
	if (!JS_GetArrayLength(cx, obj, &length))
		FAIL("Failed to get array length");
	out.reserve(length);
	for (u32 i = 0; i < length; ++i)
	{
		JS::RootedValue el(cx);
		if (!JS_GetElement(cx, obj, i, el.address()))
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
	template<> void ScriptInterface::ToJSVal<std::vector<T> >(JSContext* cx, JS::MutableHandleValue ret, const std::vector<T>& val) \
	{ \
		ToJSVal_vector(cx, ret, val); \
	} \
	template<> bool ScriptInterface::FromJSVal<std::vector<T> >(JSContext* cx, JS::HandleValue v, std::vector<T>& out) \
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
template<> void ScriptInterface::ToJSVal<std::vector<IComponent*> >(JSContext* cx, JS::MutableHandleValue ret, const std::vector<IComponent*>& val)
{
	ToJSVal_vector(cx, ret, val);
}

template<> bool ScriptInterface::FromJSVal<std::vector<Entity> >(JSContext* cx, JS::HandleValue v, std::vector<Entity>& out)
{
	return FromJSVal_vector(cx, v, out);
}
