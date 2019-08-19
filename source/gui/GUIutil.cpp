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
#include "gui/GUIManager.h"
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

	if (!GUI<T>::ParseString(m_pObject.GetGUI(), Value, settingValue))
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

template <>
bool __ParseString<bool>(const CGUI* UNUSED(pGUI), const CStrW& Value, bool& Output)
{
	if (Value == L"true")
		Output = true;
	else if (Value == L"false")
		Output = false;
	else
		return false;

	return true;
}

template <>
bool __ParseString<int>(const CGUI* UNUSED(pGUI), const CStrW& Value, int& Output)
{
	Output = Value.ToInt();
	return true;
}

template <>
bool __ParseString<u32>(const CGUI* UNUSED(pGUI), const CStrW& Value, u32& Output)
{
	Output = Value.ToUInt();
	return true;
}

template <>
bool __ParseString<float>(const CGUI* UNUSED(pGUI), const CStrW& Value, float& Output)
{
	Output = Value.ToFloat();
	return true;
}

template <>
bool __ParseString<CRect>(const CGUI* UNUSED(pGUI), const CStrW& Value, CRect& Output)
{
	const unsigned int NUM_COORDS = 4;
	float coords[NUM_COORDS];
	std::wstringstream stream;
	stream.str(Value);
	// Parse each coordinate
	for (unsigned int i = 0; i < NUM_COORDS; ++i)
	{
		if (stream.eof())
		{
			LOGWARNING("Too few CRect parameters (min %i). Your input: '%s'", NUM_COORDS, Value.ToUTF8().c_str());
			return false;
		}
		stream >> coords[i];
		if ((stream.rdstate() & std::wstringstream::failbit) != 0)
		{
			LOGWARNING("Unable to parse CRect parameters. Your input: '%s'", Value.ToUTF8().c_str());
			return false;
		}
	}

	if (!stream.eof())
	{
		LOGWARNING("Too many CRect parameters (max %i). Your input: '%s'", NUM_COORDS, Value.ToUTF8().c_str());
		return false;
	}

	// Finally the rectangle values
	Output = CRect(coords[0], coords[1], coords[2], coords[3]);

	return true;
}

template <>
bool __ParseString<CClientArea>(const CGUI* UNUSED(pGUI), const CStrW& Value, CClientArea& Output)
{
	return Output.SetClientArea(Value.ToUTF8());
}

template <>
bool __ParseString<CGUIColor>(const CGUI* pGUI, const CStrW& Value, CGUIColor& Output)
{
	return Output.ParseString(pGUI, Value.ToUTF8());
}

template <>
bool __ParseString<CSize>(const CGUI* UNUSED(pGUI), const CStrW& Value, CSize& Output)
{
	const unsigned int NUM_COORDS = 2;
	float coords[NUM_COORDS];
	std::wstringstream stream;
	stream.str(Value);
	// Parse each coordinate
	for (unsigned int i = 0; i < NUM_COORDS; ++i)
	{
		if (stream.eof())
		{
			LOGWARNING("Too few CSize parameters (min %i). Your input: '%s'", NUM_COORDS, Value.ToUTF8().c_str());
			return false;
		}
		stream >> coords[i];
		if ((stream.rdstate() & std::wstringstream::failbit) != 0)
		{
			LOGWARNING("Unable to parse CSize parameters. Your input: '%s'", Value.ToUTF8().c_str());
			return false;
		}
	}

	Output.cx = coords[0];
	Output.cy = coords[1];

	if (!stream.eof())
	{
		LOGWARNING("Too many CSize parameters (max %i). Your input: '%s'", NUM_COORDS, Value.ToUTF8().c_str());
		return false;
	}

	return true;
}

template <>
bool __ParseString<CPos>(const CGUI* UNUSED(pGUI), const CStrW& Value, CPos& Output)
{
	const unsigned int NUM_COORDS = 2;
	float coords[NUM_COORDS];
	std::wstringstream stream;
	stream.str(Value);
	// Parse each coordinate
	for (unsigned int i = 0; i < NUM_COORDS; ++i)
	{
		if (stream.eof())
		{
			LOGWARNING("Too few CPos parameters (min %i). Your input: '%s'", NUM_COORDS, Value.ToUTF8().c_str());
			return false;
		}
		stream >> coords[i];
		if ((stream.rdstate() & std::wstringstream::failbit) != 0)
		{
			LOGWARNING("Unable to parse CPos parameters. Your input: '%s'", Value.ToUTF8().c_str());
			return false;
		}
	}

	Output.x = coords[0];
	Output.y = coords[1];

	if (!stream.eof())
	{
		LOGWARNING("Too many CPos parameters (max %i). Your input: '%s'", NUM_COORDS, Value.ToUTF8().c_str());
		return false;
	}

	return true;
}

template <>
bool __ParseString<EAlign>(const CGUI* UNUSED(pGUI), const CStrW& Value, EAlign& Output)
{
	if (Value == L"left")
		Output = EAlign_Left;
	else if (Value == L"center")
		Output = EAlign_Center;
	else if (Value == L"right")
		Output = EAlign_Right;
	else
		return false;

	return true;
}

template <>
bool __ParseString<EVAlign>(const CGUI* UNUSED(pGUI), const CStrW& Value, EVAlign& Output)
{
	if (Value == L"top")
		Output = EVAlign_Top;
	else if (Value == L"center")
		Output = EVAlign_Center;
	else if (Value == L"bottom")
		Output = EVAlign_Bottom;
	else
		return false;

	return true;
}

template <>
bool __ParseString<CGUIString>(const CGUI* UNUSED(pGUI), const CStrW& Value, CGUIString& Output)
{
	Output.SetValue(Value);
	return true;
}

template <>
bool __ParseString<CStr>(const CGUI* UNUSED(pGUI), const CStrW& Value, CStr& Output)
{
	Output = Value.ToUTF8();
	return true;
}

template <>
bool __ParseString<CStrW>(const CGUI* UNUSED(pGUI), const CStrW& Value, CStrW& Output)
{
	Output = Value;
	return true;
}

template <>
bool __ParseString<CGUISpriteInstance>(const CGUI* UNUSED(pGUI), const CStrW& Value, CGUISpriteInstance& Output)
{
	Output = CGUISpriteInstance(Value.ToUTF8());
	return true;
}

template <>
bool __ParseString<CGUIList>(const CGUI* UNUSED(pGUI), const CStrW& UNUSED(Value), CGUIList& UNUSED(Output))
{
	return false;
}

template <>
bool __ParseString<CGUISeries>(const CGUI* UNUSED(pGUI), const CStrW& UNUSED(Value), CGUISeries& UNUSED(Output))
{
	return false;
}

template <typename T>
PSRETURN GUI<T>::GetSettingPointer(const IGUIObject* pObject, const CStr& Setting, T*& Value)
{
	ENSURE(pObject != NULL);

	std::map<CStr, IGUISetting*>::const_iterator it = pObject->m_Settings.find(Setting);
	if (it == pObject->m_Settings.end())
	{
		LOGWARNING("setting %s was not found on object %s",
			Setting.c_str(),
			pObject->GetPresentableName().c_str());
		return PSRETURN_GUI_InvalidSetting;
	}

	if (it->second == nullptr)
		return PSRETURN_GUI_InvalidSetting;

	// Get value
	Value = &(static_cast<CGUISetting<T>* >(it->second)->m_pSetting);

	return PSRETURN_OK;
}

template <typename T>
bool GUI<T>::HasSetting(const IGUIObject* pObject, const CStr& Setting)
{
	return pObject->m_Settings.count(Setting) != 0;
}

template <typename T>
T& GUI<T>::GetSetting(const IGUIObject* pObject, const CStr& Setting)
{
	return static_cast<CGUISetting<T>* >(pObject->m_Settings.at(Setting))->m_pSetting;
}

template <typename T>
PSRETURN GUI<T>::GetSetting(const IGUIObject* pObject, const CStr& Setting, T& Value)
{
	T* v = NULL;
	PSRETURN ret = GetSettingPointer(pObject, Setting, v);
	if (ret == PSRETURN_OK)
		Value = *v;
	return ret;
}

template <typename T>
PSRETURN GUI<T>::SetSetting(IGUIObject* pObject, const CStr& Setting, T& Value, const bool& SkipMessage)
{
	return SetSettingWrap(pObject, Setting, Value, SkipMessage,
		[&pObject, &Setting, &Value]() {
			static_cast<CGUISetting<T>* >(pObject->m_Settings[Setting])->m_pSetting = std::move(Value);
		});
}

template <typename T>
PSRETURN GUI<T>::SetSetting(IGUIObject* pObject, const CStr& Setting, const T& Value, const bool& SkipMessage)
{
	return SetSettingWrap(pObject, Setting, Value, SkipMessage,
		[&pObject, &Setting, &Value]() {
			static_cast<CGUISetting<T>* >(pObject->m_Settings[Setting])->m_pSetting = Value;
		});
}

template <typename T>
PSRETURN GUI<T>::SetSettingWrap(IGUIObject* pObject, const CStr& Setting, const T& Value, const bool& SkipMessage, const std::function<void()>& valueSet)
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
		RecurseObject(0, pObject, &IGUIObject::UpdateCachedSize);
	}
	else if (Setting == "hidden")
	{
		// Hiding an object requires us to reset it and all children
		RecurseObject(0, pObject, &IGUIObject::ResetStates);
	}

	if (!SkipMessage)
	{
		SGUIMessage msg(GUIM_SETTINGS_UPDATED, Setting);
		pObject->HandleMessage(msg);
	}

	return PSRETURN_OK;
}

// Instantiate templated functions:
// These functions avoid copies by working with a pointer and move semantics.
#define TYPE(T) \
	template bool GUI<T>::HasSetting(const IGUIObject* pObject, const CStr& Setting); \
	template T& GUI<T>::GetSetting(const IGUIObject* pObject, const CStr& Setting); \
	template PSRETURN GUI<T>::GetSettingPointer(const IGUIObject* pObject, const CStr& Setting, T*& Value); \
	template PSRETURN GUI<T>::SetSetting(IGUIObject* pObject, const CStr& Setting, T& Value, const bool& SkipMessage); \
	template class CGUISetting<T>; \

#include "GUItypes.h"
#undef TYPE

// Copying functions - discouraged except for primitives.
#define TYPE(T) \
	template PSRETURN GUI<T>::GetSetting(const IGUIObject* pObject, const CStr& Setting, T& Value); \
	template PSRETURN GUI<T>::SetSetting(IGUIObject* pObject, const CStr& Setting, const T& Value, const bool& SkipMessage); \

#define GUITYPE_IGNORE_NONCOPYABLE
#include "GUItypes.h"
#undef GUITYPE_IGNORE_NONCOPYABLE
#undef TYPE
