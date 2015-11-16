/* Copyright (C) 2015 Wildfire Games.
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

#include "CGUIScrollBarVertical.h"

#include "lib/external_libraries/libsdl.h"
#include "ps/CLogger.h"
#include "soundmanager/ISoundManager.h"


CList::CList()
	: m_Modified(false), m_PrevSelectedItem(-1), m_LastItemClickTime(0)
{
	// Add sprite_disabled! TODO

	AddSetting(GUIST_float,					"buffer_zone");
	AddSetting(GUIST_CStrW,					"font");
	AddSetting(GUIST_bool,					"scrollbar");
	AddSetting(GUIST_CStr,					"scrollbar_style");
	AddSetting(GUIST_CStrW,					"sound_disabled");
	AddSetting(GUIST_CStrW,					"sound_selected");
	AddSetting(GUIST_CGUISpriteInstance,	"sprite");
	AddSetting(GUIST_CGUISpriteInstance,	"sprite_selectarea");
	AddSetting(GUIST_int,					"cell_id");
	AddSetting(GUIST_EAlign,				"text_align");
	AddSetting(GUIST_CColor,				"textcolor");
	AddSetting(GUIST_CColor,				"textcolor_selected");
	AddSetting(GUIST_int,					"selected");	// Index selected. -1 is none.
	AddSetting(GUIST_CStrW,					"tooltip");
	AddSetting(GUIST_CStr,					"tooltip_style");
	// Each list item has both a name (in 'list') and an associated data string (in 'list_data')
	AddSetting(GUIST_CGUIList,				"list");
	AddSetting(GUIST_CGUIList,				"list_data"); // TODO: this should be a list of raw strings, not of CGUIStrings

	GUI<bool>::SetSetting(this, "scrollbar", false);

	// Nothing is selected as default.
	GUI<int>::SetSetting(this, "selected", -1);

	// Add scroll-bar
	CGUIScrollBarVertical* bar = new CGUIScrollBarVertical();
	bar->SetRightAligned(true);
	AddScrollBar(bar);
}

CList::~CList()
{
}

void CList::SetupText()
{
	if (!GetGUI())
		return;

	m_Modified = true;
	CGUIList* pList;
	GUI<CGUIList>::GetSettingPointer(this, "list", pList);

	//ENSURE(m_GeneratedTexts.size()>=1);

	m_ItemsYPositions.resize(pList->m_Items.size()+1);

	// Delete all generated texts. Some could probably be saved,
	//  but this is easier, and this function will never be called
	//  continuously, or even often, so it'll probably be okay.
	for (SGUIText* const& t : m_GeneratedTexts)
		delete t;
	m_GeneratedTexts.clear();

	CStrW font;
	if (GUI<CStrW>::GetSetting(this, "font", font) != PSRETURN_OK || font.empty())
		// Use the default if none is specified
		// TODO Gee: (2004-08-14) Don't define standard like this. Do it with the default style.
		font = L"default";

	bool scrollbar;
	GUI<bool>::GetSetting(this, "scrollbar", scrollbar);

	float width = GetListRect().GetWidth();
	// remove scrollbar if applicable
	if (scrollbar && GetScrollBar(0).GetStyle())
		width -= GetScrollBar(0).GetStyle()->m_Width;

	float buffer_zone = 0.f;
	GUI<float>::GetSetting(this, "buffer_zone", buffer_zone);

	// Generate texts
	float buffered_y = 0.f;

	for (size_t i = 0; i < pList->m_Items.size(); ++i)
	{
		// Create a new SGUIText. Later on, input it using AddText()
		SGUIText* text = new SGUIText();

		*text = GetGUI()->GenerateText(pList->m_Items[i], font, width, buffer_zone, this);
		text->
		m_ItemsYPositions[i] = buffered_y;
		buffered_y += text->m_Size.cy;

		AddText(text);
	}

	m_ItemsYPositions[pList->m_Items.size()] = buffered_y;

	// Setup scrollbar
	if (scrollbar)
	{
		GetScrollBar(0).SetScrollRange(m_ItemsYPositions.back());
		GetScrollBar(0).SetScrollSpace(GetListRect().GetHeight());

		CRect rect = GetListRect();
		GetScrollBar(0).SetX(rect.right);
		GetScrollBar(0).SetY(rect.top);
		GetScrollBar(0).SetZ(GetBufferedZ());
		GetScrollBar(0).SetLength(rect.bottom - rect.top);
	}
}

void CList::HandleMessage(SGUIMessage& Message)
{
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

			// TODO only works if lower-case, shouldn't it be made case sensitive instead?
			ScriptEvent("selectionchange");
		}

		if (Message.value == "scrollbar")
			SetupText();

		// Update scrollbar
		if (Message.value == "scrollbar_style")
		{
			CStr scrollbar_style;
			GUI<CStr>::GetSetting(this, Message.value, scrollbar_style);

			GetScrollBar(0).SetScrollBarStyle(scrollbar_style);

			SetupText();
		}

		break;

	case GUIM_MOUSE_PRESS_LEFT:
	{
		bool enabled;
		GUI<bool>::GetSetting(this, "enabled", enabled);
		if (!enabled)
		{
			CStrW soundPath;
			if (g_SoundManager && GUI<CStrW>::GetSetting(this, "sound_disabled", soundPath) == PSRETURN_OK && !soundPath.empty())
				g_SoundManager->PlayAsUI(soundPath.c_str(), false);
			break;
		}

		bool scrollbar;
		CGUIList* pList;
		GUI<bool>::GetSetting(this, "scrollbar", scrollbar);
		GUI<CGUIList>::GetSettingPointer(this, "list", pList);
		float scroll = 0.f;
		if (scrollbar)
			scroll = GetScrollBar(0).GetPos();

		CRect rect = GetListRect();
		CPos mouse = GetMousePos();
		mouse.y += scroll;
		int set = -1;
		for (int i = 0; i < (int)pList->m_Items.size(); ++i)
		{
			if (mouse.y >= rect.top + m_ItemsYPositions[i] &&
				mouse.y < rect.top + m_ItemsYPositions[i+1] &&
				// mouse is not over scroll-bar
				(!scrollbar || !GetScrollBar(0).IsVisible() ||
					mouse.x < GetScrollBar(0).GetOuterRect().left ||
					mouse.x > GetScrollBar(0).GetOuterRect().right))
			{
				set = i;
			}
		}

		if (set != -1)
		{
			GUI<int>::SetSetting(this, "selected", set);
			UpdateAutoScroll();

			CStrW soundPath;
			if (g_SoundManager && GUI<CStrW>::GetSetting(this, "sound_selected", soundPath) == PSRETURN_OK && !soundPath.empty())
				g_SoundManager->PlayAsUI(soundPath.c_str(), false);

			if (timer_Time() - m_LastItemClickTime < SELECT_DBLCLICK_RATE && set == m_PrevSelectedItem)
				this->SendEvent(GUIM_MOUSE_DBLCLICK_LEFT_ITEM, "mouseleftdoubleclickitem");
			m_LastItemClickTime = timer_Time();
			m_PrevSelectedItem = set;
		}
		break;
	}

	case GUIM_LOAD:
	{
		CStr scrollbar_style;
		GUI<CStr>::GetSetting(this, "scrollbar_style", scrollbar_style);
		GetScrollBar(0).SetScrollBarStyle(scrollbar_style);
		break;
	}

	default:
		break;
	}

	IGUITextOwner::HandleMessage(Message);
}

InReaction CList::ManuallyHandleEvent(const SDL_Event_* ev)
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

void CList::Draw()
{
	int selected;
	GUI<int>::GetSetting(this, "selected", selected);

	DrawList(selected, "sprite", "sprite_selectarea", "textcolor");
}

void CList::DrawList(const int& selected, const CStr& _sprite, const CStr& _sprite_selected, const CStr& _textcolor)
{
	float bz = GetBufferedZ();

	// First call draw on ScrollBarOwner
	bool scrollbar;
	GUI<bool>::GetSetting(this, "scrollbar", scrollbar);

	if (scrollbar)
		IGUIScrollBarOwner::Draw();

	if (GetGUI())
	{
		CRect rect = GetListRect();

		CGUISpriteInstance* sprite = NULL;
		CGUISpriteInstance* sprite_selectarea = NULL;
		int cell_id;
		GUI<CGUISpriteInstance>::GetSettingPointer(this, _sprite, sprite);
		GUI<CGUISpriteInstance>::GetSettingPointer(this, _sprite_selected, sprite_selectarea);
		GUI<int>::GetSetting(this, "cell_id", cell_id);

		CGUIList* pList;
		GUI<CGUIList>::GetSettingPointer(this, "list", pList);

		GetGUI()->DrawSprite(*sprite, cell_id, bz, rect);

		float scroll = 0.f;
		if (scrollbar)
			scroll = GetScrollBar(0).GetPos();

		if (selected != -1)
		{
			ENSURE(selected >= 0 && selected+1 < (int)m_ItemsYPositions.size());

			// Get rectangle of selection:
			CRect rect_sel(rect.left, rect.top + m_ItemsYPositions[selected] - scroll,
					       rect.right, rect.top + m_ItemsYPositions[selected+1] - scroll);

			if (rect_sel.top <= rect.bottom &&
				rect_sel.bottom >= rect.top)
			{
				if (rect_sel.bottom > rect.bottom)
					rect_sel.bottom = rect.bottom;
				if (rect_sel.top < rect.top)
					rect_sel.top = rect.top;

				if (scrollbar)
				{
					// Remove any overlapping area of the scrollbar.
					if (rect_sel.right > GetScrollBar(0).GetOuterRect().left &&
						rect_sel.right <= GetScrollBar(0).GetOuterRect().right)
						rect_sel.right = GetScrollBar(0).GetOuterRect().left;

					if (rect_sel.left >= GetScrollBar(0).GetOuterRect().left &&
						rect_sel.left < GetScrollBar(0).GetOuterRect().right)
						rect_sel.left = GetScrollBar(0).GetOuterRect().right;
				}

				GetGUI()->DrawSprite(*sprite_selectarea, cell_id, bz+0.05f, rect_sel);
			}
		}

		CColor color;
		GUI<CColor>::GetSetting(this, _textcolor, color);

		for (size_t i = 0; i < pList->m_Items.size(); ++i)
		{
			if (m_ItemsYPositions[i+1] - scroll < 0 ||
				m_ItemsYPositions[i] - scroll > rect.GetHeight())
				continue;

			// Clipping area (we'll have to substract the scrollbar)
			CRect cliparea = GetListRect();

			if (scrollbar)
			{
				if (cliparea.right > GetScrollBar(0).GetOuterRect().left &&
					cliparea.right <= GetScrollBar(0).GetOuterRect().right)
					cliparea.right = GetScrollBar(0).GetOuterRect().left;

				if (cliparea.left >= GetScrollBar(0).GetOuterRect().left &&
					cliparea.left < GetScrollBar(0).GetOuterRect().right)
					cliparea.left = GetScrollBar(0).GetOuterRect().right;
			}

			DrawText(i, color, rect.TopLeft() - CPos(0.f, scroll - m_ItemsYPositions[i]), bz+0.1f, cliparea);
		}
	}
}

void CList::AddItem(const CStrW& str, const CStrW& data)
{
	CGUIList* pList;
	CGUIList* pListData;
	GUI<CGUIList>::GetSettingPointer(this, "list", pList);
	GUI<CGUIList>::GetSettingPointer(this, "list_data", pListData);

	CGUIString gui_string;
	gui_string.SetValue(str);
	pList->m_Items.push_back(gui_string);

	CGUIString data_string;
	data_string.SetValue(data);
	pListData->m_Items.push_back(data_string);

	// TODO Temp
	SetupText();
}

bool CList::HandleAdditionalChildren(const XMBElement& child, CXeromyces* pFile)
{
	int elmt_item = pFile->GetElementID("item");

	if (child.GetNodeName() == elmt_item)
	{
		AddItem(child.GetText().FromUTF8(), child.GetText().FromUTF8());
		return true;
	}

	return false;
}

void CList::SelectNextElement()
{
	int selected;
	GUI<int>::GetSetting(this, "selected", selected);

	CGUIList* pList;
	GUI<CGUIList>::GetSettingPointer(this, "list", pList);

	if (selected != (int)pList->m_Items.size()-1)
	{
		++selected;
		GUI<int>::SetSetting(this, "selected", selected);

		CStrW soundPath;
		if (g_SoundManager && GUI<CStrW>::GetSetting(this, "sound_selected", soundPath) == PSRETURN_OK && !soundPath.empty())
			g_SoundManager->PlayAsUI(soundPath.c_str(), false);
	}
}

void CList::SelectPrevElement()
{
	int selected;
	GUI<int>::GetSetting(this, "selected", selected);

	if (selected > 0)
	{
		--selected;
		GUI<int>::SetSetting(this, "selected", selected);

		CStrW soundPath;
		if (g_SoundManager && GUI<CStrW>::GetSetting(this, "sound_selected", soundPath) == PSRETURN_OK && !soundPath.empty())
			g_SoundManager->PlayAsUI(soundPath.c_str(), false);
	}
}

void CList::SelectFirstElement()
{
	int selected;
	GUI<int>::GetSetting(this, "selected", selected);

	if (selected >= 0)
		GUI<int>::SetSetting(this, "selected", 0);
}

void CList::SelectLastElement()
{
	int selected;
	GUI<int>::GetSetting(this, "selected", selected);

	CGUIList* pList;
	GUI<CGUIList>::GetSettingPointer(this, "list", pList);

	if (selected != (int)pList->m_Items.size()-1)
		GUI<int>::SetSetting(this, "selected", (int)pList->m_Items.size()-1);
}

void CList::UpdateAutoScroll()
{
	int selected;
	bool scrollbar;
	float scroll;
	GUI<int>::GetSetting(this, "selected", selected);
	GUI<bool>::GetSetting(this, "scrollbar", scrollbar);

	CRect rect = GetListRect();

	// No scrollbar, no scrolling (at least it's not made to work properly).
	if (!scrollbar)
		return;

	scroll = GetScrollBar(0).GetPos();

	// Check upper boundary
	if (m_ItemsYPositions[selected] < scroll)
	{
		GetScrollBar(0).SetPos(m_ItemsYPositions[selected]);
		return; // this means, if it wants to align both up and down at the same time
				//  this will have precedence.
	}

	// Check lower boundary
	if (m_ItemsYPositions[selected+1]-rect.GetHeight() > scroll)
		GetScrollBar(0).SetPos(m_ItemsYPositions[selected+1]-rect.GetHeight());
}
