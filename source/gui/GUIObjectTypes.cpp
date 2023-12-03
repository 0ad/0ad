/* Copyright (C) 2020 Wildfire Games.
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

#include "gui/ObjectTypes/CButton.h"
#include "gui/ObjectTypes/CChart.h"
#include "gui/ObjectTypes/CCheckBox.h"
#include "gui/ObjectTypes/CDropDown.h"
#include "gui/ObjectTypes/CGUIDummyObject.h"
#include "gui/ObjectTypes/CHotkeyPicker.h"
#include "gui/ObjectTypes/CImage.h"
#include "gui/ObjectTypes/CInput.h"
#include "gui/ObjectTypes/CList.h"
#include "gui/ObjectTypes/CMiniMap.h"
#include "gui/ObjectTypes/COList.h"
#include "gui/ObjectTypes/CProgressBar.h"
#include "gui/ObjectTypes/CRadioButton.h"
#include "gui/ObjectTypes/CSlider.h"
#include "gui/ObjectTypes/CText.h"
#include "gui/ObjectTypes/CTooltip.h"
#include "gui/Scripting/JSInterface_GUIProxy.h"

void CGUI::AddObjectTypes()
{
	m_ProxyData.insert(JSI_GUIProxy<IGUIObject>::CreateData(*m_ScriptInterface));
	m_ProxyData.insert(JSI_GUIProxy<CText>::CreateData(*m_ScriptInterface));
	m_ProxyData.insert(JSI_GUIProxy<CList>::CreateData(*m_ScriptInterface));
	m_ProxyData.insert(JSI_GUIProxy<CMiniMap>::CreateData(*m_ScriptInterface));
	m_ProxyData.insert(JSI_GUIProxy<CButton>::CreateData(*m_ScriptInterface));

	AddObjectType("button", &CButton::ConstructObject);
	AddObjectType("chart", &CChart::ConstructObject);
	AddObjectType("checkbox", &CCheckBox::ConstructObject);
	AddObjectType("dropdown", &CDropDown::ConstructObject);
	AddObjectType("empty", &CGUIDummyObject::ConstructObject);
	AddObjectType("hotkeypicker", &CHotkeyPicker::ConstructObject);
	AddObjectType("image", &CImage::ConstructObject);
	AddObjectType("input", &CInput::ConstructObject);
	AddObjectType("list", &CList::ConstructObject);
	AddObjectType("minimap", &CMiniMap::ConstructObject);
	AddObjectType("olist", &COList::ConstructObject);
	AddObjectType("progressbar", &CProgressBar::ConstructObject);
	AddObjectType("radiobutton", &CRadioButton::ConstructObject);
	AddObjectType("slider", &CSlider::ConstructObject);
	AddObjectType("text", &CText::ConstructObject);
	AddObjectType("tooltip", &CTooltip::ConstructObject);
}
