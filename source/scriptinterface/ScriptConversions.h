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

#ifndef INCLUDED_SCRIPTCONVERSIONS
#define INCLUDED_SCRIPTCONVERSIONS

#include "ScriptRequest.h"
#include "ScriptExceptions.h"
#include "ScriptExtraHeaders.h" // for typed arrays

#include <limits>
#include <vector>

namespace Script
{
/**
 * Convert a JS::Value to a C++ type. (This might trigger GC.)
 */
template<typename T> bool FromJSVal(const ScriptRequest& rq, const JS::HandleValue val, T& ret);

/**
 * Convert a C++ type to a JS::Value. (This might trigger GC. The return
 * value must be rooted if you don't want it to be collected.)
 * NOTE: We are passing the JS::Value by reference instead of returning it by value.
 * The reason is a memory corruption problem that appears to be caused by a bug in Visual Studio.
 * Details here: http://www.wildfiregames.com/forum/index.php?showtopic=17289&p=285921
 */
template<typename T> void ToJSVal(const ScriptRequest& rq, JS::MutableHandleValue ret, T const& val);

template<>
inline void ToJSVal<JS::PersistentRootedValue>(const ScriptRequest& UNUSED(rq), JS::MutableHandleValue handle, const JS::PersistentRootedValue& a)
{
	handle.set(a);
}

template<>
inline void ToJSVal<JS::Heap<JS::Value> >(const ScriptRequest& UNUSED(rq), JS::MutableHandleValue handle, const JS::Heap<JS::Value>& a)
{
	handle.set(a);
}

template<>
inline void ToJSVal<JS::RootedValue>(const ScriptRequest& UNUSED(rq), JS::MutableHandleValue handle, const JS::RootedValue& a)
{
	handle.set(a);
}

template <>
inline void ToJSVal<JS::HandleValue>(const ScriptRequest& UNUSED(rq), JS::MutableHandleValue handle, const JS::HandleValue& a)
{
	handle.set(a);
}

/**
 * Convert a named property of an object to a C++ type.
 */
template<typename T> inline bool FromJSProperty(const ScriptRequest& rq, const JS::HandleValue val, const char* name, T& ret, bool strict = false)
{
	if (!val.isObject())
		return false;

	JS::RootedObject obj(rq.cx, &val.toObject());

	bool hasProperty;
	if (!JS_HasProperty(rq.cx, obj, name, &hasProperty) || !hasProperty)
		return false;

	JS::RootedValue value(rq.cx);
	if (!JS_GetProperty(rq.cx, obj, name, &value))
		return false;

	if (strict && value.isNull())
		return false;

	return FromJSVal(rq, value, ret);
}

template<typename T> inline void ToJSVal_vector(const ScriptRequest& rq, JS::MutableHandleValue ret, const std::vector<T>& val)
{
	JS::RootedObject obj(rq.cx, JS::NewArrayObject(rq.cx, 0));
	if (!obj)
	{
		ret.setUndefined();
		return;
	}

	ENSURE(val.size() <= std::numeric_limits<u32>::max());
	for (u32 i = 0; i < val.size(); ++i)
	{
		JS::RootedValue el(rq.cx);
		Script::ToJSVal<T>(rq, &el, val[i]);
		JS_SetElement(rq.cx, obj, i, el);
	}
	ret.setObject(*obj);
}

#define FAIL(msg) STMT(ScriptException::Raise(rq, msg); return false)

template<typename T> inline bool FromJSVal_vector(const ScriptRequest& rq, JS::HandleValue v, std::vector<T>& out)
{
	JS::RootedObject obj(rq.cx);
	if (!v.isObject())
		FAIL("Argument must be an array");

	bool isArray;
	obj = &v.toObject();
	if ((!JS::IsArrayObject(rq.cx, obj, &isArray) || !isArray) && !JS_IsTypedArrayObject(obj))
		FAIL("Argument must be an array");

	u32 length;
	if (!JS::GetArrayLength(rq.cx, obj, &length))
		FAIL("Failed to get array length");

	out.clear();
	out.reserve(length);
	for (u32 i = 0; i < length; ++i)
	{
		JS::RootedValue el(rq.cx);
		if (!JS_GetElement(rq.cx, obj, i, &el))
			FAIL("Failed to read array element");
		T el2;
		if (!Script::FromJSVal<T>(rq, el, el2))
			return false;
		out.push_back(el2);
	}
	return true;
}

#undef FAIL

#define JSVAL_VECTOR(T) \
template<> void Script::ToJSVal<std::vector<T> >(const ScriptRequest& rq, JS::MutableHandleValue ret, const std::vector<T>& val) \
{ \
	ToJSVal_vector(rq, ret, val); \
} \
template<> bool Script::FromJSVal<std::vector<T> >(const ScriptRequest& rq, JS::HandleValue v, std::vector<T>& out) \
{ \
	return FromJSVal_vector(rq, v, out); \
}

} // namespace Script

#endif //INCLUDED_SCRIPTCONVERSIONS
