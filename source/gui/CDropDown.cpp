/* Copyright (C) 2013 Wildfire Games.
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

/*
CDropDown
*/

#include "precompiled.h"

#include "CDropDown.h"

#include "ps/CLogger.h"
#include "lib/external_libraries/libsdl.h"
#include "lib/ogl.h"
#include "lib/timer.h"
#include "soundmanager/ISoundManager.h"


//-------------------------------------------------------------------
//  Constructor / Destructor
//-------------------------------------------------------------------
CDropDown::CDropDown() : m_Open(false), m_HideScrollBar(false), m_ElementHighlight(-1)
{
	AddSetting(GUIST_float,					"button_width");
	AddSetting(GUIST_float,					"dropdown_size");
	AddSetting(GUIST_float,					"dropdown_buffer");
//	AddSetting(GUIST_CStrW,					"font");
	AddSetting(GUIST_CStrW,					"sound_closed");
	AddSetting(GUIST_CStrW,					"sound_disabled");
	AddSetting(GUIST_CStrW,					"sound_enter");
	AddSetting(GUIST_CStrW,					"sound_leave");
	AddSetting(GUIST_CStrW,					"sound_opened");
//	AddSetting(GUIST_CGUISpriteInstance,	"sprite");				// Background that sits around the size
	AddSetting(GUIST_CGUISpriteInstance,	"sprite_list");			// Background of the drop down list
	AddSetting(GUIST_CGUISpriteInstance,	"sprite2");				// Button that sits to the right
	AddSetting(GUIST_CGUISpriteInstance,	"sprite2_over");
	AddSetting(GUIST_CGUISpriteInstance,	"sprite2_pressed");
	AddSetting(GUIST_CGUISpriteInstance,	"sprite2_disabled");
	AddSetting(GUIST_EVAlign,				"text_valign");
	
	// Add these in CList! And implement TODO
	//AddSetting(GUIST_CColor,				"textcolor_over");
	//AddSetting(GUIST_CColor,				"textcolor_pressed");
	//AddSetting(GUIST_CColor,				"textcolor_disabled");

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

void CDropDown::HandleMessage(SGUIMessage &Message)
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
			Message.value == "scrollbar_style" ||
			Message.value == "button_width")
		{
			SetupListRect();
		}		

		break;
	}

	case GUIM_MOUSE_MOTION:
	{
		if (m_Open)
		{
			CPos mouse = GetMousePos();

			if (GetListRect().PointInside(mouse))
			{
				bool scrollbar;
				CGUIList *pList;
				GUI<bool>::GetSetting(this, "scrollbar", scrollbar);
				GUI<CGUIList>::GetSettingPointer(this, "list", pList);
				float scroll=0.f;
				if (scrollbar)
				{
					scroll = GetScrollBar(0).GetPos();
				}

				CRect rect = GetListRect();
				mouse.y += scroll;
				int set=-1;
				for (int i=0; i<(int)pList->m_Items.size(); ++i)
				{
					if (mouse.y >= rect.top + m_ItemsYPositions[i] &&
						mouse.y < rect.top + m_ItemsYPositions[i+1] &&
						// mouse is not over scroll-bar
						!(mouse.x >= GetScrollBar(0).GetOuterRect().left &&
						mouse.x <= GetScrollBar(0).GetOuterRect().right))
					{
						set = i;
					}
				}
				
				if (set != -1)
				{
					//GUI<int>::SetSetting(this, "selected", set);
					m_ElementHighlight = set;
					//UpdateAutoScroll();
				}
			}
		}

		break;
	}

	case GUIM_MOUSE_ENTER:
	{
		bool enabled;
		GUI<bool>::GetSetting(this, "enabled", enabled);
		if (enabled)
		{
			CStrW soundPath;
			if (g_SoundManager && GUI<CStrW>::GetSetting(this, "sound_enter", soundPath) == PSRETURN_OK && !soundPath.empty())
				g_SoundManager->PlayAsUI(soundPath.c_str(), false);
		}
		break;
	}

	case GUIM_MOUSE_LEAVE:
	{
		GUI<int>::GetSetting(this, "selected", m_ElementHighlight);

		bool enabled;
		GUI<bool>::GetSetting(this, "enabled", enabled);
		if (enabled)
		{
			CStrW soundPath;
			if (g_SoundManager && GUI<CStrW>::GetSetting(this, "sound_leave", soundPath) == PSRETURN_OK && !soundPath.empty())
				g_SoundManager->PlayAsUI(soundPath.c_str(), false);
		}
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
			CStrW soundPath;
			if (g_SoundManager && GUI<CStrW>::GetSetting(this, "sound_disabled", soundPath) == PSRETURN_OK && !soundPath.empty())
				g_SoundManager->PlayAsUI(soundPath.c_str(), false);
			break;
		}

		if (!m_Open)
		{
			CGUIList *pList;
			GUI<CGUIList>::GetSettingPointer(this, "list", pList);
			if (pList->m_Items.empty())
				return;

			m_Open = true;
			GetScrollBar(0).SetZ(GetBufferedZ());
			GUI<int>::GetSetting(this, "selected", m_ElementHighlight);

			// Start at the position of the selected item, if possible.
			GetScrollBar(0).SetPos( m_ItemsYPositions.empty() ? 0 : m_ItemsYPositions[m_ElementHighlight] - 60);

			CStrW soundPath;
			if (g_SoundManager && GUI<CStrW>::GetSetting(this, "sound_opened", soundPath) == PSRETURN_OK && !soundPath.empty())
				g_SoundManager->PlayAsUI(soundPath.c_str(), false);

			return; // overshadow
		}
		else
		{
			CPos mouse = GetMousePos();

			// If the regular area is pressed, then abort, and close.
			if (m_CachedActualSize.PointInside(mouse))
			{
				m_Open = false;
				GetScrollBar(0).SetZ(GetBufferedZ());

				CStrW soundPath;
				if (g_SoundManager && GUI<CStrW>::GetSetting(this, "sound_closed", soundPath) == PSRETURN_OK && !soundPath.empty())
					g_SoundManager->PlayAsUI(soundPath.c_str(), false);

				return; // overshadow
			}

			if (!(mouse.x >= GetScrollBar(0).GetOuterRect().left &&
				mouse.x <= GetScrollBar(0).GetOuterRect().right) &&
				mouse.y >= GetListRect().top)
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

		m_ElementHighlight++;
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
		{
			CStrW soundPath;
			if (g_SoundManager && GUI<CStrW>::GetSetting(this, "sound_closed", soundPath) == PSRETURN_OK && !soundPath.empty())
				g_SoundManager->PlayAsUI(soundPath.c_str(), false);
		}
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
	int szChar = ev->ev.key.keysym.sym;
	bool update_highlight = false;

	switch (szChar)
	{
	case '\r':
		m_Open=false;
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
#if !SDL_VERSION_ATLEAST(2,0,0)
					   || (szChar >= SDLK_KP0 && szChar <= SDLK_KP9)))
#else // SDL2
					   || (szChar >= SDLK_KP_0 && szChar <= SDLK_KP_9)))
#endif
		{
			// arbitrary 1 second limit to add to string or start fresh.
			// maximal amount of characters is 100, which imo is far more than enough.
			if (timer_Time() - m_TimeOfLastInput > 1.0 || m_InputBuffer.length() >= 100)
				m_InputBuffer = szChar;
			else
				m_InputBuffer += szChar;
			
			m_TimeOfLastInput = timer_Time();
			
			CGUIList *pList;
			GUI<CGUIList>::GetSettingPointer(this, "list", pList);
			// let's look for the closest element
			// basically it's alphabetic order and "as many letters as we can get".
			int closest = -1;
			int bestIndex = -1;
			int difference = 1250;
			for (int i=0; i<(int)pList->m_Items.size(); ++i)
			{
				int indexOfDifference = 0;
				int diff = 0;
				for (size_t j=0; j < m_InputBuffer.length(); ++j)
				{
					diff = abs(pList->m_Items[i].GetOriginalString().LowerCase()[j] - (int)m_InputBuffer[j]);
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
		}
		break;
	}

	CList::ManuallyHandleEvent(ev);

	if (update_highlight)
		GUI<int>::GetSetting(this, "selected", m_ElementHighlight);

	return IN_HANDLED;
}

void CDropDown::SetupListRect()
{
	float size, buffer, button_width;
	GUI<float>::GetSetting(this, "dropdown_size", size);
	GUI<float>::GetSetting(this, "dropdown_buffer", buffer);
	GUI<float>::GetSetting(this, "button_width", button_width);

	if (m_ItemsYPositions.empty() || m_ItemsYPositions.back() >= size)
	{
		m_CachedListRect = CRect(m_CachedActualSize.left, m_CachedActualSize.bottom+buffer,
								m_CachedActualSize.right, m_CachedActualSize.bottom+buffer + size);

		m_HideScrollBar = false;
	}
	else
	{
		m_CachedListRect = CRect(m_CachedActualSize.left, m_CachedActualSize.bottom+buffer,
								 m_CachedActualSize.right - GetScrollBar(0).GetStyle()->m_Width, m_CachedActualSize.bottom+buffer + m_ItemsYPositions.back());

		// We also need to hide the scrollbar
		m_HideScrollBar = true;
	}
}

CRect CDropDown::GetListRect() const
{
	return m_CachedListRect;
}

bool CDropDown::MouseOver()
{
	if(!GetGUI())
		throw PSERROR_GUI_OperationNeedsGUIObject();

	if (m_Open)
	{
		CRect rect(m_CachedActualSize.left, m_CachedActualSize.top,
				   m_CachedActualSize.right, GetListRect().bottom);


		return rect.PointInside(GetMousePos());
	}
	else
		return m_CachedActualSize.PointInside(GetMousePos());
}

void CDropDown::Draw() 
{
	if (!GetGUI())
		return;

	float bz = GetBufferedZ();

	float dropdown_size, button_width;
	GUI<float>::GetSetting(this, "dropdown_size", dropdown_size);
	GUI<float>::GetSetting(this, "button_width", button_width);

	CGUISpriteInstance *sprite, *sprite2, *sprite2_second;
	int cell_id, selected=0;
	CColor color;

	GUI<CGUISpriteInstance>::GetSettingPointer(this, "sprite", sprite);
	GUI<CGUISpriteInstance>::GetSettingPointer(this, "sprite2", sprite2);
	GUI<int>::GetSetting(this, "cell_id", cell_id);
	GUI<int>::GetSetting(this, "selected", selected);
	GUI<CColor>::GetSetting(this, "textcolor", color);


	bool enabled;
	GUI<bool>::GetSetting(this, "enabled", enabled);

	GetGUI()->DrawSprite(*sprite, cell_id, bz, m_CachedActualSize);

	if (button_width > 0.f)
	{
		CRect rect(m_CachedActualSize.right-button_width, m_CachedActualSize.top,
				   m_CachedActualSize.right, m_CachedActualSize.bottom);

		if (!enabled)
		{
			GUI<CGUISpriteInstance>::GetSettingPointer(this, "sprite2_disabled", sprite2_second);
			GetGUI()->DrawSprite(GUI<>::FallBackSprite(*sprite2_second, *sprite2), cell_id, bz+0.05f, rect);
		}
		else
		if (m_Open)
		{
			GUI<CGUISpriteInstance>::GetSettingPointer(this, "sprite2_pressed", sprite2_second);
			GetGUI()->DrawSprite(GUI<>::FallBackSprite(*sprite2_second, *sprite2), cell_id, bz+0.05f, rect);
		}
		else
		if (m_MouseHovering)
		{
			GUI<CGUISpriteInstance>::GetSettingPointer(this, "sprite2_over", sprite2_second);
			GetGUI()->DrawSprite(GUI<>::FallBackSprite(*sprite2_second, *sprite2), cell_id, bz+0.05f, rect);
		}
		else 
			GetGUI()->DrawSprite(*sprite2, cell_id, bz+0.05f, rect);
	}

	if (selected != -1) // TODO: Maybe check validity completely?
	{
		// figure out clipping rectangle
		CRect cliparea(m_CachedActualSize.left, m_CachedActualSize.top,
					   m_CachedActualSize.right-button_width, m_CachedActualSize.bottom);

		CPos pos(m_CachedActualSize.left, m_CachedActualSize.top);
		DrawText(selected, color, pos, bz+0.1f, cliparea);
	}

	bool *scrollbar=NULL, old;
	GUI<bool>::GetSettingPointer(this, "scrollbar", scrollbar);

	old = *scrollbar;

	if (m_Open)
	{
		if (m_HideScrollBar)
            *scrollbar = false;

		DrawList(m_ElementHighlight, "sprite_list", "sprite_selectarea", "textcolor");
		
		if (m_HideScrollBar)
			*scrollbar = old;
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
