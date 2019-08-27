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

/*
	Contains help class GUI<>, which gives us templated
	 parameter to all functions within GUI.
*/

#ifndef INCLUDED_GUIUTIL
#define INCLUDED_GUIUTIL

#include "gui/IGUIObject.h"

#include <functional>

class CGUI;
template<typename T> class GUI;

class IGUISetting
{
public:
	NONCOPYABLE(IGUISetting);

	IGUISetting() = default;
	virtual ~IGUISetting() = default;

	/**
	 * Parses the given string and assigns to the setting value. Used for parsing XML attributes.
	 */
	virtual bool FromString(const CStrW& Value, const bool& SkipMessage) = 0;

	/**
	 * Parses the given JS::Value using ScriptInterface::FromJSVal and assigns it to the setting data.
	 */
	virtual bool FromJSVal(JSContext* cx, JS::HandleValue Value) = 0;

	/**
	 * Converts the setting data to a JS::Value using ScriptInterface::ToJSVal.
	 */
	virtual void ToJSVal(JSContext* cx, JS::MutableHandleValue Value) = 0;
};

template<typename T>
class CGUISetting : public IGUISetting
{
	friend class GUI<T>;

public:
	NONCOPYABLE(CGUISetting);

	CGUISetting(IGUIObject& pObject, const CStr& Name);

	/**
	 * Parses the given string and assigns to the setting value. Used for parsing XML attributes.
	 */
	bool FromString(const CStrW& Value, const bool& SkipMessage) override;

	/**
	 * Parses the given JS::Value using ScriptInterface::FromJSVal and assigns it to the setting data.
	 */
	bool FromJSVal(JSContext* cx, JS::HandleValue Value) override;

	/**
	 * Converts the setting data to a JS::Value using ScriptInterface::ToJSVal.
	 */
	void ToJSVal(JSContext* cx, JS::MutableHandleValue Value) override;

	/**
	 * These members are public because they are either unmodifiable or free to be modified.
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
	 * Holds the value of the setting.
	 */
	T m_pSetting;
};

/**
 * Includes static functions that needs one template
 * argument.
 */
template <typename T>
class GUI
{
public:
	NONCOPYABLE(GUI);


	/**
	 * Sets a value by name using a real datatype as input.
	 * This variant will use the move-assignment.
	 *
	 * @param pObject Object pointer
	 * @param Setting Setting by name
	 * @param Value Sets value to this, note type T!
	 * @param SkipMessage Does not send a GUIM_SETTINGS_UPDATED if true
	 */
	static PSRETURN SetSetting(IGUIObject* pObject, const CStr& Setting, T& Value, const bool& SkipMessage = false);

	/**
	 * This variant will copy the value.
	 */
	static PSRETURN SetSetting(IGUIObject* pObject, const CStr& Setting, const T& Value, const bool& SkipMessage = false);

	/**
	 * Sets a value by setting and object name using a real
	 * datatype as input.
	 *
	 * @param Value The value in string form, like "0 0 100% 100%"
	 * @param tOutput Parsed value of type T
	 * @return True at success.
	 */
	static bool ParseString(const CGUI* pGUI, const CStrW& Value, T& tOutput);

private:

	/**
	 * Changes the value of the setting by calling the valueSet functon that performs either a copy or move assignment.
	 * Updates some internal data depending on the setting changed.
	 */
	static PSRETURN SetSettingWrap(IGUIObject* pObject, const CStr& Setting, const bool& SkipMessage, const std::function<void()>& valueSet);
};

#endif // INCLUDED_GUIUTIL
