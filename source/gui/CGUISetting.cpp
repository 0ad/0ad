/* Copyright (C) 2023 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "precompiled.h"

#include "CGUISetting.h"

#include "gui/CGUI.h"
#include "gui/CGUISprite.h"
#include "gui/ObjectBases/IGUIObject.h"
#include "gui/SettingTypes/CGUIList.h"
#include "gui/SettingTypes/CGUISeries.h"
#include "gui/SettingTypes/CGUISize.h"
#include "gui/SettingTypes/CGUIString.h"
#include "gui/SettingTypes/EAlign.h"
#include "ps/CLogger.h"
#include "ps/CStr.h"
#include "scriptinterface/ScriptConversions.h"

IGUISetting::IGUISetting(const CStr& name, IGUIObject* owner) : m_Object(*owner), m_Name(name)
{
	m_Object.RegisterSetting(m_Name, this);
}

IGUISetting::IGUISetting(IGUISetting&& other) : m_Object(other.m_Object), m_Name(other.m_Name)
{
	m_Object.ReregisterSetting(m_Name, this);
}

bool IGUISetting::FromString(const CStrW& value, const bool sendMessage)
{
	if (!DoFromString(value))
		return false;

	OnSettingChange(GetName(), sendMessage);
	return true;
}

/**
 * Parses the given JS::Value using ScriptInterface::FromJSVal and assigns it to the setting data.
 */
bool IGUISetting::FromJSVal(const ScriptRequest& rq, JS::HandleValue value, const bool sendMessage)
{
	if (!DoFromJSVal(rq, value))
		return false;

	OnSettingChange(GetName(), sendMessage);
	return true;
}

void IGUISetting::OnSettingChange(const CStr& setting, bool sendMessage)
{
	m_Object.SettingChanged(setting, sendMessage);
}

template<typename T>
bool CGUISimpleSetting<T>::DoFromString(const CStrW& value)
{
	return CGUI::ParseString<T>(&m_Object.GetGUI(), value, m_Setting);
};

template<>
bool CGUISimpleSetting<CGUIColor>::DoFromJSVal(const ScriptRequest& rq, JS::HandleValue value)
{
	if (value.isString())
	{
		CStr name;
		if (!Script::FromJSVal(rq, value, name))
			return false;

		if (!m_Setting.ParseString(m_Object.GetGUI(), name))
		{
			LOGERROR("Invalid color '%s'", name.c_str());
			return false;
		}
		return true;
	}
	return Script::FromJSVal<CColor>(rq, value, m_Setting);
};

template<typename T>
bool CGUISimpleSetting<T>::DoFromJSVal(const ScriptRequest& rq, JS::HandleValue value)
{
	return Script::FromJSVal<T>(rq, value, m_Setting);
};

template<typename T>
void CGUISimpleSetting<T>::ToJSVal(const ScriptRequest& rq, JS::MutableHandleValue value)
{
	Script::ToJSVal<T>(rq, value, m_Setting);
};

/**
 * Explicitly instantiate CGUISimpleSetting for the basic types.
 */
#define TYPE(T) \
	template class CGUISimpleSetting<T>;

TYPE(bool)
TYPE(i32)
TYPE(u32)
TYPE(float)
TYPE(CVector2D)
TYPE(CStr)
TYPE(CStrW)
// TODO: make these inherit from CGUISimpleSetting directly.
TYPE(CGUISize)
TYPE(CGUIColor)
TYPE(CGUISpriteInstance)
TYPE(CGUIString)
TYPE(EAlign)
TYPE(EVAlign)
TYPE(CGUIList)
TYPE(CGUISeries)

#undef TYPE
