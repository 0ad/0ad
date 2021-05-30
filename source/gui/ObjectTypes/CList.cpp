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

#include "CList.h"

#include "gui/CGUI.h"
#include "gui/CGUIScrollBarVertical.h"
#include "gui/SettingTypes/CGUIColor.h"
#include "gui/SettingTypes/CGUIList.h"
#include "lib/external_libraries/libsdl.h"
#include "lib/timer.h"

const CStr CList::EventNameSelectionChange = "SelectionChange";
const CStr CList::EventNameHoverChange = "HoverChange";
const CStr CList::EventNameMouseLeftClickItem = "MouseLeftClickItem";
const CStr CList::EventNameMouseLeftDoubleClickItem = "MouseLeftDoubleClickItem";

CList::CList(CGUI& pGUI)
	: IGUIObject(pGUI),
	  IGUITextOwner(*static_cast<IGUIObject*>(this)),
	  IGUIScrollBarOwner(*static_cast<IGUIObject*>(this)),
	  m_Modified(false),
	  m_PrevSelectedItem(-1),
	  m_LastItemClickTime(0),
	  m_BufferZone(this, "buffer_zone"),
	  m_Font(this, "font"),
	  m_ScrollBar(this, "scrollbar", false),
	  m_ScrollBarStyle(this, "scrollbar_style"),
	  m_ScrollBottom(this, "scroll_bottom", false),
	  m_SoundDisabled(this, "sound_disabled"),
	  m_SoundSelected(this, "sound_selected"),
	  m_Sprite(this, "sprite"),
	  m_SpriteOverlay(this, "sprite_overlay"),
	  // Add sprite_disabled! TODO
	  m_SpriteSelectArea(this, "sprite_selectarea"),
	  m_SpriteSelectAreaOverlay(this, "sprite_selectarea_overlay"),
	  m_TextColor(this, "textcolor"),
	  m_TextColorSelected(this, "textcolor_selected"),
	  m_Selected(this, "selected", -1), // Index selected. -1 is none.
	  m_AutoScroll(this, "auto_scroll", false),
	  m_Hovered(this, "hovered", -1),
	  // Each list item has both a name (in 'list') and an associated data string (in 'list_data')
	  m_List(this, "list"),
	  m_ListData(this, "list_data")
{
	// Add scroll-bar
	CGUIScrollBarVertical* bar = new CGUIScrollBarVertical(pGUI);
	bar->SetRightAligned(true);
	AddScrollBar(bar);
}

CList::~CList()
{
}

void CList::SetupText()
{
	SetupText(false);
}

void CList::SetupText(bool append)
{
	m_Modified = true;

	if (!append)
		// Delete all generated texts.
		// TODO: try to be cleverer if we want to update items before the end.
		m_GeneratedTexts.clear();

	float width = GetListRect().GetWidth();

	bool bottom = false;
	if (m_ScrollBar && GetScrollBar(0).GetStyle())
	{
		if (m_ScrollBottom && GetScrollBar(0).GetPos() > GetScrollBar(0).GetMaxPos() - 1.5f)
			bottom = true;

		// remove scrollbar if applicable
		width -= GetScrollBar(0).GetStyle()->m_Width;
	}

	// Generate texts
	float buffered_y = 0.f;

	if (append && !m_ItemsYPositions.empty())
		buffered_y = m_ItemsYPositions.back();

	m_ItemsYPositions.resize(m_List->m_Items.size() + 1);

	for (size_t i = append ? m_List->m_Items.size() - 1 : 0; i < m_List->m_Items.size(); ++i)
	{
		CGUIText* text;

		if (!m_List->m_Items[i].GetOriginalString().empty())
			text = &AddText(m_List->m_Items[i], m_Font, width, m_BufferZone);
		else
		{
			// Minimum height of a space character of the current font size
			CGUIString align_string;
			align_string.SetValue(L" ");
			text = &AddText(align_string, m_Font, width, m_BufferZone);
		}

		m_ItemsYPositions[i] = buffered_y;
		buffered_y += text->GetSize().Height;
	}

	m_ItemsYPositions[m_List->m_Items.size()] = buffered_y;

	// Setup scrollbar
	if (m_ScrollBar)
	{
		GetScrollBar(0).SetScrollRange(m_ItemsYPositions.back());
		GetScrollBar(0).SetScrollSpace(GetListRect().GetHeight());

		CRect rect = GetListRect();
		GetScrollBar(0).SetX(rect.right);
		GetScrollBar(0).SetY(rect.top);
		GetScrollBar(0).SetZ(GetBufferedZ());
		GetScrollBar(0).SetLength(rect.bottom - rect.top);

		if (bottom)
			GetScrollBar(0).SetPos(GetScrollBar(0).GetMaxPos());
	}
}

void CList::ResetStates()
{
	IGUIObject::ResetStates();
	IGUIScrollBarOwner::ResetStates();
}

void CList::UpdateCachedSize()
{
	IGUIObject::UpdateCachedSize();
	IGUITextOwner::UpdateCachedSize();
}

void CList::HandleMessage(SGUIMessage& Message)
{
	IGUIObject::HandleMessage(Message);
	IGUIScrollBarOwner::HandleMessage(Message);
	//IGUITextOwner::HandleMessage(Message); <== placed it after the switch instead!

	m_Modified = false;
	switch (Message.type)
	{
	case GUIM_SETTINGS_UPDATED:
		if (Message.value == "list")
			SetupText();

		// If selected is changed, call "SelectionChange"
		if (Message.value == "selected")
		{
			// TODO: Check range

			if (m_AutoScroll)
				UpdateAutoScroll();

			ScriptEvent(EventNameSelectionChange);
		}

		if (Message.value == "scrollbar")
			SetupText();

		// Update scrollbar
		if (Message.value == "scrollbar_style")
		{
			GetScrollBar(0).SetScrollBarStyle(m_ScrollBarStyle);
			SetupText();
		}

		break;

	case GUIM_MOUSE_PRESS_LEFT:
	{
		if (!m_Enabled)
		{
			PlaySound(m_SoundDisabled);
			break;
		}

		int hovered = GetHoveredItem();
		if (hovered == -1)
			break;
		m_Selected.Set(hovered, true);
		UpdateAutoScroll();
		PlaySound(m_SoundSelected);

		if (timer_Time() - m_LastItemClickTime < SELECT_DBLCLICK_RATE && hovered == m_PrevSelectedItem)
			this->SendMouseEvent(GUIM_MOUSE_DBLCLICK_LEFT_ITEM, EventNameMouseLeftDoubleClickItem);
		else
			this->SendMouseEvent(GUIM_MOUSE_PRESS_LEFT_ITEM, EventNameMouseLeftClickItem);

		m_LastItemClickTime = timer_Time();
		m_PrevSelectedItem = hovered;
		break;
	}

	case GUIM_MOUSE_LEAVE:
	{
		if (m_Hovered == -1)
			break;

		m_Hovered.Set(-1, true);
		ScriptEvent(EventNameHoverChange);
		break;
	}

	case GUIM_MOUSE_OVER:
	{
		int hovered = GetHoveredItem();
		if (hovered == m_Hovered)
			break;

		m_Hovered.Set(hovered, true);
		ScriptEvent(EventNameHoverChange);
		break;
	}

	case GUIM_LOAD:
	{
		GetScrollBar(0).SetScrollBarStyle(m_ScrollBarStyle);
		break;
	}

	default:
		break;
	}

	IGUITextOwner::HandleMessage(Message);
}

InReaction CList::ManuallyHandleKeys(const SDL_Event_* ev)
{
	InReaction result = IN_PASS;

	if (ev->ev.type == SDL_KEYDOWN)
	{
		int szChar = ev->ev.key.keysym.sym;

		switch (szChar)
		{
		case SDLK_HOME:
			SelectFirstElement();
			UpdateAutoScroll();
			result = IN_HANDLED;
			break;

		case SDLK_END:
			SelectLastElement();
			UpdateAutoScroll();
			result = IN_HANDLED;
			break;

		case SDLK_UP:
			SelectPrevElement();
			UpdateAutoScroll();
			result = IN_HANDLED;
			break;

		case SDLK_DOWN:
			SelectNextElement();
			UpdateAutoScroll();
			result = IN_HANDLED;
			break;

		case SDLK_PAGEUP:
			GetScrollBar(0).ScrollMinusPlenty();
			result = IN_HANDLED;
			break;

		case SDLK_PAGEDOWN:
			GetScrollBar(0).ScrollPlusPlenty();
			result = IN_HANDLED;
			break;

		default: // Do nothing
			result = IN_PASS;
		}
	}

	return result;
}

void CList::Draw(CCanvas2D& canvas)
{
	DrawList(canvas, m_Selected, m_Sprite, m_SpriteOverlay, m_SpriteSelectArea, m_SpriteSelectAreaOverlay, m_TextColor);
}

void CList::DrawList(CCanvas2D& canvas, const int& selected, const CGUISpriteInstance& sprite, const CGUISpriteInstance& spriteOverlay,
                     const CGUISpriteInstance& spriteSelectArea, const CGUISpriteInstance& spriteSelectAreaOverlay, const CGUIColor& textColor)
{
	CRect rect = GetListRect();

	m_pGUI.DrawSprite(sprite, canvas, rect);

	float scroll = 0.f;
	if (m_ScrollBar)
		scroll = GetScrollBar(0).GetPos();

	bool drawSelected = false;
	CRect rectSel;
	if (selected >= 0 && selected+1 < (int)m_ItemsYPositions.size())
	{
		// Get rectangle of selection:
		rectSel = CRect(
			rect.left, rect.top + m_ItemsYPositions[selected] - scroll,
			rect.right, rect.top + m_ItemsYPositions[selected+1] - scroll);

		if (rectSel.top <= rect.bottom &&
			rectSel.bottom >= rect.top)
		{
			if (rectSel.bottom > rect.bottom)
				rectSel.bottom = rect.bottom;
			if (rectSel.top < rect.top)
				rectSel.top = rect.top;
			if (m_ScrollBar)
			{
				// Remove any overlapping area of the scrollbar.
				if (rectSel.right > GetScrollBar(0).GetOuterRect().left &&
					rectSel.right <= GetScrollBar(0).GetOuterRect().right)
					rectSel.right = GetScrollBar(0).GetOuterRect().left;

				if (rectSel.left >= GetScrollBar(0).GetOuterRect().left &&
					rectSel.left < GetScrollBar(0).GetOuterRect().right)
					rectSel.left = GetScrollBar(0).GetOuterRect().right;
			}

			m_pGUI.DrawSprite(spriteSelectArea, canvas, rectSel);
			drawSelected = true;
		}
	}

	for (size_t i = 0; i < m_List->m_Items.size(); ++i)
	{
		if (m_ItemsYPositions[i+1] - scroll < 0 ||
			m_ItemsYPositions[i] - scroll > rect.GetHeight())
			continue;

		// Clipping area (we'll have to substract the scrollbar)
		CRect cliparea = GetListRect();

		if (m_ScrollBar)
		{
			if (cliparea.right > GetScrollBar(0).GetOuterRect().left &&
				cliparea.right <= GetScrollBar(0).GetOuterRect().right)
				cliparea.right = GetScrollBar(0).GetOuterRect().left;

			if (cliparea.left >= GetScrollBar(0).GetOuterRect().left &&
				cliparea.left < GetScrollBar(0).GetOuterRect().right)
				cliparea.left = GetScrollBar(0).GetOuterRect().right;
		}

		DrawText(canvas, i, textColor, rect.TopLeft() - CVector2D(0.f, scroll - m_ItemsYPositions[i]), cliparea);
	}

	// Draw scrollbars on top of the content
	if (m_ScrollBar)
		IGUIScrollBarOwner::Draw(canvas);

	// Draw the overlays last
	m_pGUI.DrawSprite(spriteOverlay, canvas, rect);
	if (drawSelected)
		m_pGUI.DrawSprite(spriteSelectAreaOverlay, canvas, rectSel);
}

void CList::AddItem(const CGUIString& str, const CGUIString& data)
{
	// Do not send a settings-changed message
	m_List.GetMutable().m_Items.push_back(str);
	m_ListData.GetMutable().m_Items.push_back(data);

	SetupText(true);
}

void CList::AddItem(const CGUIString& strAndData)
{
	AddItem(strAndData, strAndData);
}

bool CList::HandleAdditionalChildren(const XMBData& xmb, const XMBElement& child)
{
	int elmt_item = xmb.GetElementID("item");

	if (child.GetNodeName() == elmt_item)
	{
		CGUIString vlist;
		vlist.SetValue(child.GetText().FromUTF8());
		AddItem(vlist, vlist);
		return true;
	}

	return false;
}

void CList::SelectNextElement()
{
	if (m_Selected != static_cast<int>(m_List->m_Items.size()) - 1)
	{
		m_Selected.Set(m_Selected + 1, true);
		PlaySound(m_SoundSelected);
	}
}

void CList::SelectPrevElement()
{
	if (m_Selected > 0)
	{
		m_Selected.Set(m_Selected - 1, true);
		PlaySound(m_SoundSelected);
	}
}

void CList::SelectFirstElement()
{
	if (m_Selected >= 0)
		m_Selected.Set(0, true);
}

void CList::SelectLastElement()
{
	const int index = static_cast<int>(m_List->m_Items.size()) - 1;

	if (m_Selected != index)
		m_Selected.Set(index, true);
}

void CList::UpdateAutoScroll()
{
	// No scrollbar, no scrolling (at least it's not made to work properly).
	if (!m_ScrollBar || m_Selected < 0 || static_cast<std::size_t>(m_Selected) >= m_ItemsYPositions.size())
		return;

	float scroll = GetScrollBar(0).GetPos();

	// Check upper boundary
	if (m_ItemsYPositions[m_Selected] < scroll)
	{
		GetScrollBar(0).SetPos(m_ItemsYPositions[m_Selected]);
		return; // this means, if it wants to align both up and down at the same time
				//  this will have precedence.
	}

	// Check lower boundary
	CRect rect = GetListRect();
	if (m_ItemsYPositions[m_Selected+1]-rect.GetHeight() > scroll)
		GetScrollBar(0).SetPos(m_ItemsYPositions[m_Selected+1]-rect.GetHeight());
}

int CList::GetHoveredItem()
{
	const float scroll = m_ScrollBar ? GetScrollBar(0).GetPos() : 0.f;

	const CRect& rect = GetListRect();
	CVector2D mouse = m_pGUI.GetMousePos();
	mouse.Y += scroll;

	// Mouse is over scrollbar
	if (m_ScrollBar && GetScrollBar(0).IsVisible() &&
	    mouse.X >= GetScrollBar(0).GetOuterRect().left &&
	    mouse.X <= GetScrollBar(0).GetOuterRect().right)
		return -1;

	for (size_t i = 0; i < m_List->m_Items.size(); ++i)
		if (mouse.Y >= rect.top + m_ItemsYPositions[i] &&
		    mouse.Y < rect.top + m_ItemsYPositions[i + 1])
			return i;

	return -1;
}
