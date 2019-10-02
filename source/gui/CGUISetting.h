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

#ifndef INCLUDED_CGUISETTINGS
#define INCLUDED_CGUISETTINGS

#include "gui/ObjectBases/IGUIObject.h"

/**
 * This setting interface allows GUI objects to call setting function functions without having to know the setting type.
 * This is fact is used for setting the value from a JS value or XML value (string) and when deleting the setting,
 * when the type of the setting value is not known in advance.
 */
class IGUISetting
{
public:
	NONCOPYABLE(IGUISetting);

	IGUISetting() = default;
	virtual ~IGUISetting() = default;

	/**
	 * Parses the given string and assigns to the setting value. Used for parsing XML attributes.
	 */
	virtual bool FromString(const CStrW& Value, const bool SendMessage) = 0;

	/**
	 * Parses the given JS::Value using ScriptInterface::FromJSVal and assigns it to the setting data.
	 */
	virtual bool FromJSVal(JSContext* cx, JS::HandleValue Value, const bool SendMessage) = 0;

	/**
	 * Converts the setting data to a JS::Value using ScriptInterface::ToJSVal.
	 */
	virtual void ToJSVal(JSContext* cx, JS::MutableHandleValue Value) = 0;
};

template<typename T>
class CGUISetting : public IGUISetting
{
public:
	NONCOPYABLE(CGUISetting);

	CGUISetting(IGUIObject& pObject, const CStr& Name, T& Value);

	/**
	 * Parses the given string and assigns to the setting value. Used for parsing XML attributes.
	 */
	bool FromString(const CStrW& Value, const bool SendMessage) override;

	/**
	 * Parses the given JS::Value using ScriptInterface::FromJSVal and assigns it to the setting data.
	 */
	bool FromJSVal(JSContext* cx, JS::HandleValue Value, const bool SendMessage) override;

	/**
	 * Converts the setting data to a JS::Value using ScriptInterface::ToJSVal.
	 */
	void ToJSVal(JSContext* cx, JS::MutableHandleValue Value) override;

	/**
	 * These members are public because they are either unmodifiable or free to be modified.
	 * In particular it avoids the need for setter templates specialized depending on copiability.
	 */

	/**
	 * The object that stores this setting.
	 */
	IGUIObject& m_pObject;

	/**
	 * Property name identifying the setting.
	 */
	const CStr m_Name;

	/**
	 * Holds a reference to the value of the setting.
	 * The setting value is stored in the member class to optimize for draw calls of that class.
	 */
	T& m_pSetting;
};

#endif // INCLUDED_CGUISETTINGS
