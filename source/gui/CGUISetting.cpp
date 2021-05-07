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

#include "precompiled.h"

#include "CGUISetting.h"

#include "gui/CGUI.h"
#include "ps/CLogger.h"
#include "scriptinterface/ScriptInterface.h"

IGUISetting::IGUISetting(const CStr& name, IGUIObject* owner) : m_pObject(*owner)
{
	m_pObject.RegisterSetting(name, this);
}

IGUISetting::IGUISetting(IGUISetting&& o) : m_pObject(o.m_pObject)
{
	m_pObject.ReregisterSetting(o.GetName(), this);
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
	m_pObject.SettingChanged(setting, sendMessage);
}

template<typename T>
bool CGUISimpleSetting<T>::DoFromString(const CStrW& value)
{
	return CGUI::ParseString<T>(&m_pObject.GetGUI(), value, m_Setting);
};

template<>
bool CGUISimpleSetting<CGUIColor>::DoFromJSVal(const ScriptRequest& rq, JS::HandleValue value)
{
	if (value.isString())
	{
		CStr name;
		if (!ScriptInterface::FromJSVal(rq, value, name))
			return false;

		if (!m_Setting.ParseString(m_pObject.GetGUI(), name))
		{
			LOGERROR("Invalid color '%s'", name.c_str());
			return false;
		}
	}

	return ScriptInterface::FromJSVal<CColor>(rq, value, m_Setting);
};

template<typename T>
bool CGUISimpleSetting<T>::DoFromJSVal(const ScriptRequest& rq, JS::HandleValue value)
{
	return ScriptInterface::FromJSVal<T>(rq, value, m_Setting);
};

template<typename T>
void CGUISimpleSetting<T>::ToJSVal(const ScriptRequest& rq, JS::MutableHandleValue value)
{
	ScriptInterface::ToJSVal<T>(rq, value, m_Setting);
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
#include "ps/CStr.h"
TYPE(CStr)
TYPE(CStrW)
// TODO: make these inherit from CGUISimpleSetting directly.
#include "gui/SettingTypes/CGUISize.h"
TYPE(CGUISize)
TYPE(CGUIColor)
#include "gui/CGUISprite.h"
TYPE(CGUISpriteInstance)
#include "gui/SettingTypes/CGUIString.h"
TYPE(CGUIString)
#include "gui/SettingTypes/EAlign.h"
TYPE(EAlign)
TYPE(EVAlign)
#include "gui/SettingTypes/CGUIList.h"
TYPE(CGUIList)
#include "gui/SettingTypes/CGUISeries.h"
TYPE(CGUISeries)

#undef TYPE
