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

#include "CDropDown.h"

#include "gui/CGUIColor.h"
#include "lib/external_libraries/libsdl.h"
#include "lib/ogl.h"
#include "lib/timer.h"
#include "ps/CLogger.h"

CDropDown::CDropDown(CGUI& pGUI)
	: CList(pGUI), IGUIObject(pGUI),
	  m_Open(false), m_HideScrollBar(false), m_ElementHighlight(-1)
{
	AddSetting<float>("button_width");
	AddSetting<float>("dropdown_size");
	AddSetting<float>("dropdown_buffer");
	AddSetting<uint>("minimum_visible_items");
//	AddSetting<CStrW, "font");
	AddSetting<CStrW>("sound_closed");
	AddSetting<CStrW>("sound_disabled");
	AddSetting<CStrW>("sound_enter");
	AddSetting<CStrW>("sound_leave");
	AddSetting<CStrW>("sound_opened");
	AddSetting<CGUISpriteInstance>("sprite");				// Background that sits around the size
	AddSetting<CGUISpriteInstance>("sprite_disabled");
	AddSetting<CGUISpriteInstance>("sprite_list");			// Background of the drop down list
	AddSetting<CGUISpriteInstance>("sprite2");				// Button that sits to the right
	AddSetting<CGUISpriteInstance>("sprite2_over");
	AddSetting<CGUISpriteInstance>("sprite2_pressed");
	AddSetting<CGUISpriteInstance>("sprite2_disabled");
	AddSetting<EVAlign>("text_valign");

	// Add these in CList! And implement TODO
	//AddSetting<CGUIColor>("textcolor_over");
	//AddSetting<CGUIColor>("textcolor_pressed");
	AddSetting<CGUIColor>("textcolor_selected");
	AddSetting<CGUIColor>("textcolor_disabled");

	// Scrollbar is forced to be true.
	GUI<bool>::SetSetting(this, "scrollbar", true);
}

CDropDown::~CDropDown()
{
}

void CDropDown::SetupText()
{
	SetupListRect();
	CList::SetupText();
}

void CDropDown::UpdateCachedSize()
{
	CList::UpdateCachedSize();
	SetupText();
}

void CDropDown::HandleMessage(SGUIMessage& Message)
{
	// Important

	switch (Message.type)
	{
	case GUIM_SETTINGS_UPDATED:
	{
		// Update cached list rect
		if (Message.value == "size" ||
			Message.value == "absolute" ||
			Message.value == "dropdown_size" ||
			Message.value == "dropdown_buffer" ||
			Message.value == "minimum_visible_items" ||
			Message.value == "scrollbar_style" ||
			Message.value == "button_width")
		{
			SetupListRect();
		}

		break;
	}

	case GUIM_MOUSE_MOTION:
	{
		if (!m_Open)
			break;

		CPos mouse = m_pGUI.GetMousePos();

		if (!GetListRect().PointInside(mouse))
			break;

		bool scrollbar;
		const CGUIList& pList = GUI<CGUIList>::GetSetting(this, "list");
		GUI<bool>::GetSetting(this, "scrollbar", scrollbar);
		float scroll = 0.f;
		if (scrollbar)
			scroll = GetScrollBar(0).GetPos();

		CRect rect = GetListRect();
		mouse.y += scroll;
		int set = -1;
		for (int i = 0; i < static_cast<int>(pList.m_Items.size()); ++i)
		{
			if (mouse.y >= rect.top + m_ItemsYPositions[i] &&
				mouse.y < rect.top + m_ItemsYPositions[i+1] &&
				// mouse is not over scroll-bar
				(m_HideScrollBar ||
					mouse.x < GetScrollBar(0).GetOuterRect().left ||
					mouse.x > GetScrollBar(0).GetOuterRect().right))
			{
				set = i;
			}
		}

		if (set != -1)
		{
			m_ElementHighlight = set;
			//UpdateAutoScroll();
		}

		break;
	}

	case GUIM_MOUSE_ENTER:
	{
		bool enabled;
		GUI<bool>::GetSetting(this, "enabled", enabled);
		if (enabled)
			PlaySound("sound_enter");
		break;
	}

	case GUIM_MOUSE_LEAVE:
	{
		GUI<int>::GetSetting(this, "selected", m_ElementHighlight);

		bool enabled;
		GUI<bool>::GetSetting(this, "enabled", enabled);
		if (enabled)
			PlaySound("sound_leave");
		break;
	}

	// We can't inherent this routine from CList, because we need to include
	// a mouse click to open the dropdown, also the coordinates are changed.
	case GUIM_MOUSE_PRESS_LEFT:
	{
		bool enabled;
		GUI<bool>::GetSetting(this, "enabled", enabled);
		if (!enabled)
		{
			PlaySound("sound_disabled");
			break;
		}

		if (!m_Open)
		{
			const CGUIList& pList = GUI<CGUIList>::GetSetting(this, "list");
			if (pList.m_Items.empty())
				return;

			m_Open = true;
			GetScrollBar(0).SetZ(GetBufferedZ());
			GUI<int>::GetSetting(this, "selected", m_ElementHighlight);

			// Start at the position of the selected item, if possible.
			GetScrollBar(0).SetPos(m_ItemsYPositions.empty() ? 0 : m_ItemsYPositions[m_ElementHighlight] - 60);

			PlaySound("sound_opened");
			return; // overshadow
		}
		else
		{
			const CPos& mouse = m_pGUI.GetMousePos();

			// If the regular area is pressed, then abort, and close.
			if (m_CachedActualSize.PointInside(mouse))
			{
				m_Open = false;
				GetScrollBar(0).SetZ(GetBufferedZ());
				PlaySound("sound_closed");
				return; // overshadow
			}

			if (m_HideScrollBar ||
				mouse.x < GetScrollBar(0).GetOuterRect().left ||
				mouse.x > GetScrollBar(0).GetOuterRect().right ||
				mouse.y < GetListRect().top)
			{
				m_Open = false;
				GetScrollBar(0).SetZ(GetBufferedZ());
			}
		}
		break;
	}

	case GUIM_MOUSE_WHEEL_DOWN:
	{
		bool enabled;
		GUI<bool>::GetSetting(this, "enabled", enabled);

		// Don't switch elements by scrolling when open, causes a confusing interaction between this and the scrollbar.
		if (m_Open || !enabled)
			break;

		GUI<int>::GetSetting(this, "selected", m_ElementHighlight);
		if (m_ElementHighlight + 1 >= (int)m_ItemsYPositions.size() - 1)
			break;

		++m_ElementHighlight;
		GUI<int>::SetSetting(this, "selected", m_ElementHighlight);
		break;
	}

	case GUIM_MOUSE_WHEEL_UP:
	{
		bool enabled;
		GUI<bool>::GetSetting(this, "enabled", enabled);

		// Don't switch elements by scrolling when open, causes a confusing interaction between this and the scrollbar.
		if (m_Open || !enabled)
			break;

		GUI<int>::GetSetting(this, "selected", m_ElementHighlight);
		if (m_ElementHighlight - 1 < 0)
			break;

		m_ElementHighlight--;
		GUI<int>::SetSetting(this, "selected", m_ElementHighlight);
		break;
	}

	case GUIM_LOST_FOCUS:
	{
		if (m_Open)
			PlaySound("sound_closed");

		m_Open = false;
		break;
	}

	case GUIM_LOAD:
		SetupListRect();
		break;

	default:
		break;
	}

	// Important that this is after, so that overshadowed implementations aren't processed
	CList::HandleMessage(Message);

	// As HandleMessage functions return void, we need to manually verify
	// whether the child list's items were modified.
	if (CList::GetModified())
		SetupText();
}

InReaction CDropDown::ManuallyHandleEvent(const SDL_Event_* ev)
{
	InReaction result = IN_PASS;
	bool update_highlight = false;

	if (ev->ev.type == SDL_KEYDOWN)
	{
		int szChar = ev->ev.key.keysym.sym;

		switch (szChar)
		{
		case '\r':
			m_Open = false;
			result = IN_HANDLED;
			break;

		case SDLK_HOME:
		case SDLK_END:
		case SDLK_UP:
		case SDLK_DOWN:
		case SDLK_PAGEUP:
		case SDLK_PAGEDOWN:
			if (!m_Open)
				return IN_PASS;
			// Set current selected item to highlighted, before
			//  then really processing these in CList::ManuallyHandleEvent()
			GUI<int>::SetSetting(this, "selected", m_ElementHighlight);
			update_highlight = true;
			break;

		default:
			// If we have inputed a character try to get the closest element to it.
			// TODO: not too nice and doesn't deal with dashes.
			if (m_Open && ((szChar >= SDLK_a && szChar <= SDLK_z) || szChar == SDLK_SPACE
						   || (szChar >= SDLK_0 && szChar <= SDLK_9)
						   || (szChar >= SDLK_KP_0 && szChar <= SDLK_KP_9)))
			{
				// arbitrary 1 second limit to add to string or start fresh.
				// maximal amount of characters is 100, which imo is far more than enough.
				if (timer_Time() - m_TimeOfLastInput > 1.0 || m_InputBuffer.length() >= 100)
					m_InputBuffer = szChar;
				else
					m_InputBuffer += szChar;

				m_TimeOfLastInput = timer_Time();

				const CGUIList& pList = GUI<CGUIList>::GetSetting(this, "list");
				// let's look for the closest element
				// basically it's alphabetic order and "as many letters as we can get".
				int closest = -1;
				int bestIndex = -1;
				int difference = 1250;
				for (int i = 0; i < static_cast<int>(pList.m_Items.size()); ++i)
				{
					int indexOfDifference = 0;
					int diff = 0;
					for (size_t j = 0; j < m_InputBuffer.length(); ++j)
					{
						diff = std::abs(static_cast<int>(pList.m_Items[i].GetRawString().LowerCase()[j]) - static_cast<int>(m_InputBuffer[j]));
						if (diff == 0)
							indexOfDifference = j+1;
						else
							break;
					}
					if (indexOfDifference > bestIndex || (indexOfDifference >= bestIndex && diff < difference))
					{
						bestIndex = indexOfDifference;
						closest = i;
						difference = diff;
					}
				}
				// let's select the closest element. There should basically always be one.
				if (closest != -1)
				{
					GUI<int>::SetSetting(this, "selected", closest);
					update_highlight = true;
					GetScrollBar(0).SetPos(m_ItemsYPositions[closest] - 60);
				}
				result = IN_HANDLED;
			}
			break;
		}
	}

	if (CList::ManuallyHandleEvent(ev) == IN_HANDLED)
		result = IN_HANDLED;

	if (update_highlight)
		GUI<int>::GetSetting(this, "selected", m_ElementHighlight);

	return result;
}

void CDropDown::SetupListRect()
{
	extern int g_yres;
	extern float g_GuiScale;
	float size, buffer, yres;
	yres = g_yres / g_GuiScale;
	u32 minimumVisibleItems;
	GUI<float>::GetSetting(this, "dropdown_size", size);
	GUI<float>::GetSetting(this, "dropdown_buffer", buffer);
	GUI<u32>::GetSetting(this, "minimum_visible_items", minimumVisibleItems);

	if (m_ItemsYPositions.empty())
	{
		m_CachedListRect = CRect(m_CachedActualSize.left, m_CachedActualSize.bottom + buffer,
		                         m_CachedActualSize.right, m_CachedActualSize.bottom + buffer + size);
		m_HideScrollBar = false;
	}
	// Too many items so use a scrollbar
	else if (m_ItemsYPositions.back() > size)
	{
		// Place items below if at least some items can be placed below
		if (m_CachedActualSize.bottom + buffer + size <= yres)
			m_CachedListRect = CRect(m_CachedActualSize.left, m_CachedActualSize.bottom + buffer,
			                         m_CachedActualSize.right, m_CachedActualSize.bottom + buffer + size);
		else if ((m_ItemsYPositions.size() > minimumVisibleItems && yres - m_CachedActualSize.bottom - buffer >= m_ItemsYPositions[minimumVisibleItems]) ||
		         m_CachedActualSize.top < yres - m_CachedActualSize.bottom)
			m_CachedListRect = CRect(m_CachedActualSize.left, m_CachedActualSize.bottom + buffer,
			                         m_CachedActualSize.right, yres);
		// Not enough space below, thus place items above
		else
			m_CachedListRect = CRect(m_CachedActualSize.left, std::max(0.f, m_CachedActualSize.top - buffer - size),
			                         m_CachedActualSize.right, m_CachedActualSize.top - buffer);

		m_HideScrollBar = false;
	}
	else
	{
		// Enough space below, no scrollbar needed
		if (m_CachedActualSize.bottom + buffer + m_ItemsYPositions.back() <= yres)
		{
			m_CachedListRect = CRect(m_CachedActualSize.left, m_CachedActualSize.bottom + buffer,
			                         m_CachedActualSize.right, m_CachedActualSize.bottom + buffer + m_ItemsYPositions.back());
			m_HideScrollBar = true;
		}
		// Enough space below for some items, but not all, so place items below and use a scrollbar
		else if ((m_ItemsYPositions.size() > minimumVisibleItems && yres - m_CachedActualSize.bottom - buffer >= m_ItemsYPositions[minimumVisibleItems]) ||
		         m_CachedActualSize.top < yres - m_CachedActualSize.bottom)
		{
			m_CachedListRect = CRect(m_CachedActualSize.left, m_CachedActualSize.bottom + buffer,
			                         m_CachedActualSize.right, yres);
			m_HideScrollBar = false;
		}
		// Not enough space below, thus place items above. Hide the scrollbar accordingly
		else
		{
			m_CachedListRect = CRect(m_CachedActualSize.left, std::max(0.f, m_CachedActualSize.top - buffer - m_ItemsYPositions.back()),
			                         m_CachedActualSize.right, m_CachedActualSize.top - buffer);
			m_HideScrollBar = m_CachedActualSize.top > m_ItemsYPositions.back() + buffer;
		}
	}
}

CRect CDropDown::GetListRect() const
{
	return m_CachedListRect;
}

bool CDropDown::MouseOver()
{
	if (m_Open)
	{
		CRect rect(m_CachedActualSize.left, std::min(m_CachedActualSize.top, GetListRect().top),
		           m_CachedActualSize.right, std::max(m_CachedActualSize.bottom, GetListRect().bottom));
		return rect.PointInside(m_pGUI.GetMousePos());
	}
	else
		return m_CachedActualSize.PointInside(m_pGUI.GetMousePos());
}

void CDropDown::Draw()
{
	float bz = GetBufferedZ();

	float dropdown_size, button_width;
	GUI<float>::GetSetting(this, "dropdown_size", dropdown_size);
	GUI<float>::GetSetting(this, "button_width", button_width);

	int cell_id, selected = 0;

	bool enabled;
	GUI<bool>::GetSetting(this, "enabled", enabled);

	CGUISpriteInstance& sprite = GUI<CGUISpriteInstance>::GetSetting(this, enabled ? "sprite" : "sprite_disabled");
	CGUISpriteInstance& sprite2 = GUI<CGUISpriteInstance>::GetSetting(this, "sprite2");

	GUI<int>::GetSetting(this, "cell_id", cell_id);
	GUI<int>::GetSetting(this, "selected", selected);

	m_pGUI.DrawSprite(sprite, cell_id, bz, m_CachedActualSize);

	if (button_width > 0.f)
	{
		CRect rect(m_CachedActualSize.right-button_width, m_CachedActualSize.top,
				   m_CachedActualSize.right, m_CachedActualSize.bottom);

		if (!enabled)
		{
			CGUISpriteInstance& sprite2_second = GUI<CGUISpriteInstance>::GetSetting(this, "sprite2_disabled");
			m_pGUI.DrawSprite(sprite2_second || sprite2, cell_id, bz + 0.05f, rect);
		}
		else if (m_Open)
		{
			CGUISpriteInstance& sprite2_second = GUI<CGUISpriteInstance>::GetSetting(this, "sprite2_pressed");
			m_pGUI.DrawSprite(sprite2_second || sprite2, cell_id, bz + 0.05f, rect);
		}
		else if (m_MouseHovering)
		{
			CGUISpriteInstance& sprite2_second = GUI<CGUISpriteInstance>::GetSetting(this, "sprite2_over");
			m_pGUI.DrawSprite(sprite2_second || sprite2, cell_id, bz + 0.05f, rect);
		}
		else
			m_pGUI.DrawSprite(sprite2, cell_id, bz + 0.05f, rect);
	}

	if (selected != -1) // TODO: Maybe check validity completely?
	{
		CRect cliparea(m_CachedActualSize.left, m_CachedActualSize.top,
					   m_CachedActualSize.right-button_width, m_CachedActualSize.bottom);

		const CGUIColor& color = GUI<CGUIColor>::GetSetting(this, enabled ? "textcolor_selected" : "textcolor_disabled");

		CPos pos(m_CachedActualSize.left, m_CachedActualSize.top);
		DrawText(selected, color, pos, bz+0.1f, cliparea);
	}

	// Disable scrollbar during drawing without sending a setting-changed message
	bool& scrollbar = GUI<bool>::GetSetting(this, "scrollbar");
	bool old = scrollbar;

	if (m_Open)
	{
		if (m_HideScrollBar)
			scrollbar = false;

		DrawList(m_ElementHighlight, "sprite_list", "sprite_selectarea", "textcolor");

		if (m_HideScrollBar)
			scrollbar = old;
	}
}

// When a dropdown list is opened, it needs to be visible above all the other
// controls on the page. The only way I can think of to do this is to increase
// its z value when opened, so that it's probably on top.
float CDropDown::GetBufferedZ() const
{
	float bz = CList::GetBufferedZ();
	if (m_Open)
		return std::min(bz + 500.f, 1000.f); // TODO - don't use magic number for max z value
	else
		return bz;
}
