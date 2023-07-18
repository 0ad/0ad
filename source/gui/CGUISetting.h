/* Copyright (C) 2023 Wildfire Games.
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

#include "ps/CStr.h"
#include "scriptinterface/ScriptForward.h"


class IGUIObject;

/**
 * This setting interface allows GUI objects to call setting function functions without having to know the setting type.
 * This is fact is used for setting the value from a JS value or XML value (string) and when deleting the setting,
 * when the type of the setting value is not known in advance.
 */
class IGUISetting
{
public:
	NONCOPYABLE(IGUISetting);
	IGUISetting(const CStr& name, IGUIObject* owner);

	/**
	 * Parses the given string and assigns to the setting value. Used for parsing XML attributes.
	 */
	bool FromString(const CStrW& value, const bool sendMessage);

	/**
	 * Parses the given JS::Value using Script::FromJSVal and assigns it to the setting data.
	 */
	bool FromJSVal(const ScriptRequest& rq, JS::HandleValue value, const bool sendMessage);

	/**
	 * Converts the setting data to a JS::Value using Script::ToJSVal.
	 */
	virtual void ToJSVal(const ScriptRequest& rq, JS::MutableHandleValue value) = 0;

protected:
	IGUISetting(IGUISetting&& other);
	IGUISetting& operator=(IGUISetting&& other) = delete;

	virtual ~IGUISetting() = default;

	virtual bool DoFromString(const CStrW& value) = 0;
	virtual bool DoFromJSVal(const ScriptRequest& rq, JS::HandleValue value) = 0;

	/**
	 * Triggers the IGUIObject logic when a setting changes.
	 * This should be called by derived classes when something externally visible changes,
	 * unless overloaded to provide similar behaviour.
	 */
	virtual void OnSettingChange(const CStr& setting, bool sendMessage);

	/**
	 * Return the name of the setting, from JS.
	 */
	const CStr& GetName() const
	{
		return m_Name;
	}

	/**
	 * The object that stores this setting.
	 */
	IGUIObject& m_Object;

private:
	CStr m_Name;
};

/**
 * Wraps a T. Makes sure the appropriate setting functions are called when modifying T,
 * and likewise makes sure that JS/xml settings affect T appropriately,
 * while being as transparent as possible to use from C++ code.
 */
template<typename T>
class CGUISimpleSetting : public IGUISetting
{
public:
	template<typename... Args>
	CGUISimpleSetting(IGUIObject* pObject, const CStr& name, Args&&... args)
		: IGUISetting(name, pObject), m_Setting(args...)
	{}
	NONCOPYABLE(CGUISimpleSetting);
	CGUISimpleSetting(CGUISimpleSetting&&) = default;
	CGUISimpleSetting& operator=(CGUISimpleSetting&&) = delete;

	operator const T&() const { return m_Setting; }
	const T& operator*() const { return m_Setting; }
	const T* operator->() const { return &m_Setting; }

	/**
	 * 'Uglified' getter when you want direct access without triggering messages.
	 */
	T& GetMutable() { return m_Setting; }

	/**
	 * 'Uglified' operator=, so that SendMessage is explicit.
	 */
	void Set(T value, bool sendMessage)
	{
		m_Setting = std::move(value);
		OnSettingChange(GetName(), sendMessage);
	}

protected:
	bool DoFromString(const CStrW& value) override;
	bool DoFromJSVal(const ScriptRequest& rq, JS::HandleValue value) override;
	void ToJSVal(const ScriptRequest& rq, JS::MutableHandleValue value) override;

	T m_Setting;
};

#endif // INCLUDED_CGUISETTINGS
