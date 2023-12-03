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

#ifndef INCLUDED_SCRIPTINTERFACE_OBJECT
#define INCLUDED_SCRIPTINTERFACE_OBJECT

#include "ScriptConversions.h"
#include "ScriptRequest.h"
#include "ScriptTypes.h"

#include "ps/CLogger.h"

/**
 * Wraps SM APIs for manipulating JS objects.
 */

namespace Script
{
/**
 * Get the named property on the given object.
 */
template<typename PropType>
inline bool GetProperty(const ScriptRequest& rq, JS::HandleValue obj, PropType name, JS::MutableHandleValue out)
{
	if (!obj.isObject())
		return false;
	JS::RootedObject object(rq.cx, &obj.toObject());
	if constexpr (std::is_same_v<int, PropType>)
	{
		JS::RootedId id(rq.cx, INT_TO_JSID(name));
		return JS_GetPropertyById(rq.cx, object, id, out);
	}
	else if constexpr (std::is_same_v<const char*, PropType>)
		return JS_GetProperty(rq.cx, object, name, out);
	else
		return JS_GetUCProperty(rq.cx, object, name, wcslen(name), out);
}

template<typename T, typename PropType>
inline bool GetProperty(const ScriptRequest& rq, JS::HandleValue obj, PropType name, T& out)
{
	JS::RootedValue val(rq.cx);
	if (!GetProperty<PropType>(rq, obj, name, &val))
		return false;
	return FromJSVal(rq, val, out);
}
inline bool GetProperty(const ScriptRequest& rq, JS::HandleValue obj, const char* name, JS::MutableHandleObject out)
{
	JS::RootedValue val(rq.cx, JS::ObjectValue(*out.get()));
	if (!GetProperty(rq, obj, name, &val))
		return false;
	out.set(val.toObjectOrNull());
	return true;
}

template<typename T>
inline bool GetPropertyInt(const ScriptRequest& rq, JS::HandleValue obj, int name, T& out)
{
	return GetProperty(rq, obj, name, out);
}
inline bool GetPropertyInt(const ScriptRequest& rq, JS::HandleValue obj, int name, JS::MutableHandleValue out)
{
	return GetProperty(rq, obj, name, out);
}
/**
 * Check the named property has been defined on the given object.
 */
inline bool HasProperty(const ScriptRequest& rq, JS::HandleValue obj, const char* name)
{
	if (!obj.isObject())
		return false;
	JS::RootedObject object(rq.cx, &obj.toObject());

	bool found;
	if (!JS_HasProperty(rq.cx, object, name, &found))
		return false;
	return found;
}

/**
 * Set the named property on the given object.
 */
template<typename PropType>
inline bool SetProperty(const ScriptRequest& rq, JS::HandleValue obj, PropType name, JS::HandleValue value, bool constant = false, bool enumerable = true)
{
	uint attrs = 0;
	if (constant)
		attrs |= JSPROP_READONLY | JSPROP_PERMANENT;
	if (enumerable)
		attrs |= JSPROP_ENUMERATE;

	if (!obj.isObject())
		return false;
	JS::RootedObject object(rq.cx, &obj.toObject());
	if constexpr (std::is_same_v<int, PropType>)
	{
		JS::RootedId id(rq.cx, INT_TO_JSID(name));
		return JS_DefinePropertyById(rq.cx, object, id, value, attrs);
	}
	else if constexpr (std::is_same_v<const char*, PropType>)
		return JS_DefineProperty(rq.cx, object, name, value, attrs);
	else
		return JS_DefineUCProperty(rq.cx, object, name, value, attrs);
}

template<typename T, typename PropType>
inline bool SetProperty(const ScriptRequest& rq, JS::HandleValue obj, PropType name, const T& value, bool constant = false, bool enumerable = true)
{
	JS::RootedValue val(rq.cx);
	Script::ToJSVal(rq, &val, value);
	return SetProperty<PropType>(rq, obj, name, val, constant, enumerable);
}

template<typename T>
inline bool SetPropertyInt(const ScriptRequest& rq, JS::HandleValue obj, int name, const T& value, bool constant = false, bool enumerable = true)
{
	return SetProperty<T, int>(rq, obj, name, value, constant, enumerable);
}

template<typename T>
inline bool GetObjectClassName(const ScriptRequest& rq, JS::HandleObject obj, T& name)
{
	JS::RootedValue constructor(rq.cx, JS::ObjectOrNullValue(JS_GetConstructor(rq.cx, obj)));
	return constructor.isObject() && Script::HasProperty(rq, constructor, "name") && Script::GetProperty(rq, constructor, "name", name);
}

/**
 * Get the name of the object's class. Note that inheritance may lead to unexpected results.
 */
template<typename T>
inline bool GetObjectClassName(const ScriptRequest& rq, JS::HandleValue val, T& name)
{
	JS::RootedObject obj(rq.cx, val.toObjectOrNull());
	if (!obj)
		return false;
	return GetObjectClassName(rq, obj, name);
}

inline bool FreezeObject(const ScriptRequest& rq, JS::HandleValue objVal, bool deep)
{
	if (!objVal.isObject())
		return false;

	JS::RootedObject obj(rq.cx, &objVal.toObject());

	if (deep)
		return JS_DeepFreezeObject(rq.cx, obj);
	else
		return JS_FreezeObject(rq.cx, obj);
}

/**
 * Returns all properties of the object, both own properties and inherited.
 * This is essentially equivalent to calling Object.getOwnPropertyNames()
 * and recursing up the prototype chain.
 * NB: this does not return properties with symbol or numeric keys, as that would
 * require a variant in the vector, and it's not useful for now.
 * @param enumerableOnly - only return enumerable properties.
 */
inline bool EnumeratePropertyNames(const ScriptRequest& rq, JS::HandleValue objVal, bool enumerableOnly, std::vector<std::string>& out)
{
	if (!objVal.isObjectOrNull())
	{
		LOGERROR("EnumeratePropertyNames expected object type!");
		return false;
	}

	JS::RootedObject obj(rq.cx, &objVal.toObject());
	JS::RootedIdVector props(rq.cx);
	// This recurses up the prototype chain on its own.
	if (!js::GetPropertyKeys(rq.cx, obj, enumerableOnly? 0 : JSITER_HIDDEN, &props))
		return false;

	out.reserve(out.size() + props.length());
	for (size_t i = 0; i < props.length(); ++i)
	{
		JS::RootedId id(rq.cx, props[i]);
		JS::RootedValue val(rq.cx);
		if (!JS_IdToValue(rq.cx, id, &val))
			return false;

		// Ignore integer properties for now.
		// TODO: is this actually a thing in ECMAScript 6?
		if (!val.isString())
			continue;

		std::string propName;
		if (!FromJSVal(rq, val, propName))
			return false;

		out.emplace_back(std::move(propName));
	}

	return true;
}

/**
 * Create a plain object (i.e. {}). If it fails, returns undefined.
 */
inline JS::Value CreateObject(const ScriptRequest& rq)
{
	JS::RootedObject obj(rq.cx, JS_NewPlainObject(rq.cx));
	if (!obj)
		return JS::UndefinedValue();
	return JS::ObjectValue(*obj.get());
}

inline bool CreateObject(const ScriptRequest& rq, JS::MutableHandleValue objectValue)
{
	objectValue.set(CreateObject(rq));
	return !objectValue.isNullOrUndefined();
}

/**
 * Sets the given value to a new plain JS::Object, converts the arguments to JS::Values and sets them as properties.
 * This is static so that callers like ToJSVal can use it with the JSContext directly instead of having to obtain the instance using GetScriptInterfaceAndCBData.
 * Can throw an exception.
 */
template<typename T, typename... Args>
inline bool CreateObject(const ScriptRequest& rq, JS::MutableHandleValue objectValue, const char* propertyName, const T& propertyValue, Args const&... args)
{
	JS::RootedValue val(rq.cx);
	ToJSVal(rq, &val, propertyValue);
	return CreateObject(rq, objectValue, args...) && SetProperty(rq, objectValue, propertyName, val, false, true);
}

/**
 * Sets the given value to a new JS object or Null Value in case of out-of-memory.
 */
inline bool CreateArray(const ScriptRequest& rq, JS::MutableHandleValue objectValue, size_t length = 0)
{
	objectValue.setObjectOrNull(JS::NewArrayObject(rq.cx, length));
	return !objectValue.isNullOrUndefined();
}

} // namespace Script

#endif // INCLUDED_SCRIPTINTERFACE_Object
