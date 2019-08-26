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

#include "GUIutil.h"

#include "gui/GUI.h"
#include "ps/CLogger.h"

template<typename T>
CGUISetting<T>::CGUISetting(IGUIObject& pObject, const CStr& Name)
	: m_pSetting(T()), m_Name(Name), m_pObject(pObject)
{
}

template<typename T>
bool CGUISetting<T>::FromString(const CStrW& Value, const bool& SkipMessage)
{
	T settingValue;

	if (!GUI<T>::ParseString(&m_pObject.GetGUI(), Value, settingValue))
		return false;

	GUI<T>::SetSetting(&m_pObject, m_Name, settingValue, SkipMessage);
	return true;
};

template<>
bool CGUISetting<CGUIColor>::FromJSVal(JSContext* cx, JS::HandleValue Value)
{
	CGUIColor settingValue;
	if (Value.isString())
	{
		CStr name;
		if (!ScriptInterface::FromJSVal(cx, Value, name))
			return false;

		if (!settingValue.ParseString(m_pObject.GetGUI(), name))
		{
			JS_ReportError(cx, "Invalid color '%s'", name.c_str());
			return false;
		}
	}
	else if (!ScriptInterface::FromJSVal<CColor>(cx, Value, settingValue))
		return false;

	GUI<CGUIColor>::SetSetting(&m_pObject, m_Name, settingValue);
	return true;
};

template<typename T>
bool CGUISetting<T>::FromJSVal(JSContext* cx, JS::HandleValue Value)
{
	T settingValue;
	if (!ScriptInterface::FromJSVal<T>(cx, Value, settingValue))
		return false;

	GUI<T>::SetSetting(&m_pObject, m_Name, settingValue);
	return true;
};

template<typename T>
void CGUISetting<T>::ToJSVal(JSContext* cx, JS::MutableHandleValue Value)
{
	ScriptInterface::ToJSVal<T>(cx, Value, m_pSetting);
};

template <typename T>
PSRETURN GUI<T>::SetSetting(IGUIObject* pObject, const CStr& Setting, T& Value, const bool& SkipMessage)
{
	return SetSettingWrap(pObject, Setting, SkipMessage,
		[&pObject, &Setting, &Value]() {
			static_cast<CGUISetting<T>* >(pObject->m_Settings[Setting])->m_pSetting = std::move(Value);
		});
}

template <typename T>
PSRETURN GUI<T>::SetSetting(IGUIObject* pObject, const CStr& Setting, const T& Value, const bool& SkipMessage)
{
	return SetSettingWrap(pObject, Setting, SkipMessage,
		[&pObject, &Setting, &Value]() {
			static_cast<CGUISetting<T>* >(pObject->m_Settings[Setting])->m_pSetting = Value;
		});
}

template <typename T>
PSRETURN GUI<T>::SetSettingWrap(IGUIObject* pObject, const CStr& Setting, const bool& SkipMessage, const std::function<void()>& valueSet)
{
	ENSURE(pObject != NULL);

	if (!pObject->SettingExists(Setting))
	{
		LOGWARNING("setting %s was not found on object %s",
			Setting.c_str(),
			pObject->GetPresentableName().c_str());
		return PSRETURN_GUI_InvalidSetting;
	}

	valueSet();

	//	Some settings needs special attention at change

	// If setting was "size", we need to re-cache itself and all children
	if (Setting == "size")
	{
		pObject->RecurseObject(nullptr, &IGUIObject::UpdateCachedSize);
	}
	else if (Setting == "hidden")
	{
		// Hiding an object requires us to reset it and all children
		if (pObject->GetSetting<bool>(Setting))
			pObject->RecurseObject(nullptr, &IGUIObject::ResetStates);
	}

	if (!SkipMessage)
	{
		SGUIMessage msg(GUIM_SETTINGS_UPDATED, Setting);
		pObject->HandleMessage(msg);
	}

	return PSRETURN_OK;
}

// Instantiate templated functions:
// These functions avoid copies by working with a reference and move semantics.
#define TYPE(T) \
	template PSRETURN GUI<T>::SetSetting(IGUIObject* pObject, const CStr& Setting, T& Value, const bool& SkipMessage); \
	template class CGUISetting<T>; \

#include "GUItypes.h"
#undef TYPE

// Copying functions - discouraged except for primitives.
#define TYPE(T) \
	template PSRETURN GUI<T>::SetSetting(IGUIObject* pObject, const CStr& Setting, const T& Value, const bool& SkipMessage); \

#define GUITYPE_IGNORE_NONCOPYABLE
#include "GUItypes.h"
#undef GUITYPE_IGNORE_NONCOPYABLE
#undef TYPE
