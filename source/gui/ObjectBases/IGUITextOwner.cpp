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

#include "IGUITextOwner.h"

#include "gui/CGUI.h"
#include "gui/SettingTypes/CGUIString.h"

#include <math.h>

IGUITextOwner::IGUITextOwner(IGUIObject& pObject)
	: m_pObject(pObject),
	  m_GeneratedTextsValid()
{
}

IGUITextOwner::~IGUITextOwner()
{
}

CGUIText& IGUITextOwner::AddText()
{
	m_GeneratedTexts.emplace_back();
	return m_GeneratedTexts.back();
}

CGUIText& IGUITextOwner::AddText(const CGUIString& Text, const CStrW& Font, const float& Width, const float& BufferZone)
{
	// Avoids a move constructor
	m_GeneratedTexts.emplace_back(m_pObject.GetGUI(), Text, Font, Width, BufferZone, &m_pObject);
	return m_GeneratedTexts.back();
}

void IGUITextOwner::HandleMessage(SGUIMessage& Message)
{
	switch (Message.type)
	{
	case GUIM_SETTINGS_UPDATED:
		// Everything that can change the visual appearance.
		//  it is assumed that the text of the object will be dependent on
		//  these. Although that is not certain, but one will have to manually
		//  change it and disregard this function.
		// TODO Gee: (2004-09-07) Make sure this is all options that can affect the text.
		if (Message.value == "size" || Message.value == "z" ||
			Message.value == "absolute" || Message.value == "caption" ||
			Message.value == "font" || Message.value == "textcolor" ||
			Message.value == "text_align" || Message.value == "text_valign" ||
			Message.value == "buffer_zone")
		{
			m_GeneratedTextsValid = false;
		}
		break;

	default:
		break;
	}
}

void IGUITextOwner::UpdateCachedSize()
{
	// update our text positions
	m_GeneratedTextsValid = false;
}

void IGUITextOwner::UpdateText()
{
	if (!m_GeneratedTextsValid)
	{
		SetupText();
		m_GeneratedTextsValid = true;
	}
}

void IGUITextOwner::DrawText(size_t index, const CGUIColor& color, const CPos& pos, float z, const CRect& clipping)
{
	UpdateText();

	ENSURE(index < m_GeneratedTexts.size() && "Trying to draw a Text Index within a IGUITextOwner that doesn't exist");

	m_GeneratedTexts.at(index).Draw(m_pObject.GetGUI(), color, pos, z, clipping);
}

void IGUITextOwner::CalculateTextPosition(CRect& ObjSize, CPos& TextPos, CGUIText& Text)
{
	// The horizontal Alignment is now computed in GenerateText in order to not have to
	// loop through all of the TextCall objects again.
	TextPos.x = ObjSize.left;

	switch (m_pObject.GetSetting<EVAlign>("text_valign"))
	{
	case EVAlign_Top:
		TextPos.y = ObjSize.top;
		break;
	case EVAlign_Center:
		// Round to integer pixel values, else the fonts look awful
		TextPos.y = floorf(ObjSize.CenterPoint().y - Text.GetSize().cy / 2.f);
		break;
	case EVAlign_Bottom:
		TextPos.y = ObjSize.bottom - Text.GetSize().cy;
		break;
	default:
		debug_warn(L"Broken EVAlign in CButton::SetupText()");
		break;
	}
}

bool IGUITextOwner::MouseOverIcon()
{
	return false;
}
