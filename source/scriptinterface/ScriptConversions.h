/* Copyright (C) 2017 Wildfire Games.
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

#ifndef INCLUDED_SCRIPTCONVERSIONS
#define INCLUDED_SCRIPTCONVERSIONS

#include "ScriptInterface.h"
#include "scriptinterface/ScriptExtraHeaders.h" // for typed arrays

#include <limits>

template<typename T> static void ToJSVal_vector(JSContext* cx, JS::MutableHandleValue ret, const std::vector<T>& val)
{
	JSAutoRequest rq(cx);
	JS::RootedObject obj(cx, JS_NewArrayObject(cx, 0));
	if (!obj)
	{
		ret.setUndefined();
		return;
	}

	ENSURE(val.size() <= std::numeric_limits<u32>::max());
	for (u32 i = 0; i < val.size(); ++i)
	{
		JS::RootedValue el(cx);
		ScriptInterface::ToJSVal<T>(cx, &el, val[i]);
		JS_SetElement(cx, obj, i, el);
	}
	ret.setObject(*obj);
}

#define FAIL(msg) STMT(JS_ReportError(cx, msg); return false)

template<typename T> static bool FromJSVal_vector(JSContext* cx, JS::HandleValue v, std::vector<T>& out)
{
	JSAutoRequest rq(cx);
	JS::RootedObject obj(cx);
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
		if (!JS_GetElement(cx, obj, i, &el))
			FAIL("Failed to read array element");
		T el2;
		if (!ScriptInterface::FromJSVal<T>(cx, el, el2))
			return false;
		out.push_back(el2);
	}
	return true;
}

#undef FAIL

#define JSVAL_VECTOR(T) \
template<> void ScriptInterface::ToJSVal<std::vector<T> >(JSContext* cx, JS::MutableHandleValue ret, const std::vector<T>& val) \
{ \
	ToJSVal_vector(cx, ret, val); \
} \
template<> bool ScriptInterface::FromJSVal<std::vector<T> >(JSContext* cx, JS::HandleValue v, std::vector<T>& out) \
{ \
	return FromJSVal_vector(cx, v, out); \
}

template<typename T> static bool FromJSProperty(JSContext* cx, JS::HandleValue v, const char* name, T& out)
{
	if (!v.isObject())
		return false;

	JSAutoRequest rq(cx);
	JS::RootedObject obj(cx, &v.toObject());

	bool hasProperty;
	if (!JS_HasProperty(cx, obj, name, &hasProperty))
		return false;

	JS::RootedValue value(cx);
	if (!hasProperty || !JS_GetProperty(cx, obj, name, &value))
		return false;

	if (!ScriptInterface::FromJSVal(cx, value, out))
		return false;

	return true;
}

#endif //INCLUDED_SCRIPTCONVERSIONS
