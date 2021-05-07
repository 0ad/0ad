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

#include "CGUIHotkey.h"

#include "gui/ObjectBases/IGUIObject.h"
#include "scriptinterface/ScriptInterface.h"

bool CGUIHotkey::DoFromString(const CStrW& value)
{
	m_pObject.GetGUI().UnsetObjectHotkey(&m_pObject, m_Setting);
	m_Setting = value.ToUTF8();
	m_pObject.GetGUI().SetObjectHotkey(&m_pObject, m_Setting);
	return true;
}

bool CGUIHotkey::DoFromJSVal(const ScriptRequest& rq, JS::HandleValue value)
{
	m_pObject.GetGUI().UnsetObjectHotkey(&m_pObject, m_Setting);
	if (!ScriptInterface::FromJSVal(rq, value, m_Setting))
		return false;
	m_pObject.GetGUI().SetObjectHotkey(&m_pObject, m_Setting);
	return true;
}

void CGUIHotkey::OnSettingChange(const CStr& setting, bool sendMessage)
{
	IGUISetting::OnSettingChange(setting, sendMessage);
}

