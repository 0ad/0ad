/* Copyright (C) 2010 Wildfire Games.
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
CInput
*/

#include "precompiled.h"
#include "GUI.h"
#include "CInput.h"
#include "CGUIScrollBarVertical.h"

#include "ps/Font.h"
#include "lib/ogl.h"

#include "lib/res/graphics/unifont.h"
#include "lib/sysdep/clipboard.h"

#include "ps/Hotkey.h"
#include "ps/CLogger.h"
#include "ps/Globals.h"


//-------------------------------------------------------------------
//  Constructor / Destructor
//-------------------------------------------------------------------
CInput::CInput() : m_iBufferPos(-1), m_iBufferPos_Tail(-1), m_SelectingText(false), m_HorizontalScroll(0.f)
{
	AddSetting(GUIST_float,					"buffer_zone");
	AddSetting(GUIST_CStrW,					"caption");
	AddSetting(GUIST_int,					"cell_id");
	AddSetting(GUIST_CStrW,					"font");
	AddSetting(GUIST_int,					"max_length");
	AddSetting(GUIST_bool,					"multiline");
	AddSetting(GUIST_bool,					"scrollbar");
	AddSetting(GUIST_CStr,					"scrollbar_style");
	AddSetting(GUIST_CGUISpriteInstance,	"sprite");
	AddSetting(GUIST_CGUISpriteInstance,	"sprite_selectarea");
	AddSetting(GUIST_CColor,				"textcolor");
	AddSetting(GUIST_CColor,				"textcolor_selected");
	AddSetting(GUIST_CStrW,					"tooltip");
	AddSetting(GUIST_CStr,					"tooltip_style");

	// Add scroll-bar
	CGUIScrollBarVertical * bar = new CGUIScrollBarVertical();
	bar->SetRightAligned(true);
	bar->SetUseEdgeButtons(true);
	AddScrollBar(bar);
}

CInput::~CInput()
{
}

InReaction CInput::ManuallyHandleEvent(const SDL_Event_* ev)
{
	ENSURE(m_iBufferPos != -1);

	// Since the GUI framework doesn't handle to set settings
	//  in Unicode (CStrW), we'll simply retrieve the actual
	//  pointer and edit that.
	CStrW *pCaption = (CStrW*)m_Settings["caption"].m_pSetting;

	if (ev->ev.type == SDL_HOTKEYDOWN)
	{
		std::string hotkey = static_cast<const char*>(ev->ev.user.data1);
		if (hotkey == "console.paste")
		{
			wchar_t* text = sys_clipboard_get();
			if (text)
			{
				if (m_iBufferPos == (int)pCaption->length())
					*pCaption += text;
				else
					*pCaption = pCaption->Left(m_iBufferPos) + text + 
					pCaption->Right((long) pCaption->length()-m_iBufferPos);

				UpdateText(m_iBufferPos, m_iBufferPos, m_iBufferPos+1);

				m_iBufferPos += (int)wcslen(text);

				sys_clipboard_free(text);
			}

			return IN_HANDLED;
		}
	}
	else if (ev->ev.type == SDL_KEYDOWN)
	{
		int szChar = ev->ev.key.keysym.sym;
		wchar_t cooked = (wchar_t)ev->ev.key.keysym.unicode;

		switch (szChar)
		{
			case '\t':
				/* Auto Complete */
				// TODO Gee: (2004-09-07) What to do with tab?
				break;

			case '\b':
				m_WantedX=0.f;

				if (SelectingText())
					DeleteCurSelection();
				else
				{
					m_iBufferPos_Tail = -1;

					if (pCaption->empty() ||
						m_iBufferPos == 0)
						break;

					if (m_iBufferPos == (int)pCaption->length())
						*pCaption = pCaption->Left( (long) pCaption->length()-1);
					else
						*pCaption = pCaption->Left( m_iBufferPos-1 ) + 
									pCaption->Right( (long) pCaption->length()-m_iBufferPos );

					--m_iBufferPos;
					
					UpdateText(m_iBufferPos, m_iBufferPos+1, m_iBufferPos);
				}

				UpdateAutoScroll();
				break;

			case SDLK_DELETE:
				m_WantedX=0.f;
				// If selection:
				if (SelectingText())
				{
					DeleteCurSelection();
				}
				else
				{
					if (pCaption->empty() ||
						m_iBufferPos == (int)pCaption->length())
						break;

					*pCaption = pCaption->Left( m_iBufferPos ) + 
								pCaption->Right( (long) pCaption->length()-(m_iBufferPos+1) );

					UpdateText(m_iBufferPos, m_iBufferPos+1, m_iBufferPos);
				}

				UpdateAutoScroll();
				break;

			case SDLK_HOME:
				// If there's not a selection, we should create one now
				if (!g_keys[SDLK_RSHIFT] && !g_keys[SDLK_LSHIFT])
				{
					// Make sure a selection isn't created.
					m_iBufferPos_Tail = -1;
				}
				else if (!SelectingText())
				{
					// Place tail at the current point:
					m_iBufferPos_Tail = m_iBufferPos;
				}

				m_iBufferPos = 0;
				m_WantedX=0.f;

				UpdateAutoScroll();
				break;

			case SDLK_END:
				// If there's not a selection, we should create one now
				if (!g_keys[SDLK_RSHIFT] && !g_keys[SDLK_LSHIFT])
				{
					// Make sure a selection isn't created.
					m_iBufferPos_Tail = -1;
				}
				else if (!SelectingText())
				{
					// Place tail at the current point:
					m_iBufferPos_Tail = m_iBufferPos;
				}

				m_iBufferPos = (long) pCaption->length();
				m_WantedX=0.f;

				UpdateAutoScroll();
				break;

			/**
				Conventions for Left/Right when text is selected:

				References:

				Visual Studio
					Visual Studio has the 'newer' approach, used by newer versions of
					things, and in newer applications. A left press will always place
					the pointer on the left edge of the selection, and then of course
					remove the selection. Right will do the exakt same thing.
					If you have the pointer on the right edge and press right, it will
					in other words just remove the selection.

				Windows (eg. Notepad)
					A left press always takes the pointer a step to the left and
					removes the selection as if it were never there in the first place.
					Right of course does the same thing but to the right.

				I chose the Visual Studio convention. Used also in Word, gtk 2.0, MSN
				Messenger.

			**/
			case SDLK_LEFT:
				// reset m_WantedX, very important
				m_WantedX=0.f;

				if (g_keys[SDLK_RSHIFT] || g_keys[SDLK_LSHIFT] ||
					!SelectingText())
				{
					// If there's not a selection, we should create one now
					if (!SelectingText() && !g_keys[SDLK_RSHIFT] && !g_keys[SDLK_LSHIFT])
					{
						// Make sure a selection isn't created.
						m_iBufferPos_Tail = -1;
					}
					else if (!SelectingText())
					{
						// Place tail at the current point:
						m_iBufferPos_Tail = m_iBufferPos;
					}

					if (m_iBufferPos) 
						--m_iBufferPos;
				}
				else
				{
					if (m_iBufferPos_Tail < m_iBufferPos)
						m_iBufferPos = m_iBufferPos_Tail;

					m_iBufferPos_Tail = -1;
				}

				UpdateAutoScroll();
				break;

			case SDLK_RIGHT:
				m_WantedX=0.f;

				if (g_keys[SDLK_RSHIFT] || g_keys[SDLK_LSHIFT] || 
					!SelectingText())
				{
					// If there's not a selection, we should create one now
					if (!SelectingText() && !g_keys[SDLK_RSHIFT] && !g_keys[SDLK_LSHIFT])
					{
						// Make sure a selection isn't created.
						m_iBufferPos_Tail = -1;
					}
					else if (!SelectingText())
					{
						// Place tail at the current point:
						m_iBufferPos_Tail = m_iBufferPos;
					}


					if (m_iBufferPos != (int)pCaption->length())
						++m_iBufferPos;
				}
				else
				{
					if (m_iBufferPos_Tail > m_iBufferPos)
						m_iBufferPos = m_iBufferPos_Tail;

					m_iBufferPos_Tail = -1;
				}			

				UpdateAutoScroll();
				break;

			/**
				Conventions for Up/Down when text is selected:

				References:

				Visual Studio
					Visual Studio has a very strange approach, down takes you below the
					selection to the next row, and up to the one prior to the whole
					selection. The weird part is that it is always aligned as the
					'pointer'. I decided this is to much work for something that is
					a bit arbitrary

				Windows (eg. Notepad)
					Just like with left/right, the selection is destroyed and it moves
					just as if there never were a selection.

				I chose the Notepad convention even though I use the VS convention with
				left/right.

			**/
			case SDLK_UP:
			{
				// If there's not a selection, we should create one now
				if (!g_keys[SDLK_RSHIFT] && !g_keys[SDLK_LSHIFT])
				{
					// Make sure a selection isn't created.
					m_iBufferPos_Tail = -1;
				}
				else if (!SelectingText())
				{
					// Place tail at the current point:
					m_iBufferPos_Tail = m_iBufferPos;
				}

				std::list<SRow>::iterator current = m_CharacterPositions.begin();
				while (current != m_CharacterPositions.end())
				{
					if (m_iBufferPos >= current->m_ListStart &&
						m_iBufferPos <= current->m_ListStart+(int)current->m_ListOfX.size())
						break;
					
					++current;
				}

				float pos_x;

				if (m_iBufferPos-current->m_ListStart == 0)
					pos_x = 0.f;
				else
					pos_x = current->m_ListOfX[m_iBufferPos-current->m_ListStart-1];

				if (m_WantedX > pos_x)
					pos_x = m_WantedX;

				// Now change row:
				if (current != m_CharacterPositions.begin())
				{
					--current;

					// Find X-position:
					m_iBufferPos = current->m_ListStart + GetXTextPosition(current, pos_x, m_WantedX);
				}
				// else we can't move up
				
				UpdateAutoScroll();
			}
				break;

			case SDLK_DOWN:
			{
				// If there's not a selection, we should create one now
				if (!g_keys[SDLK_RSHIFT] && !g_keys[SDLK_LSHIFT])
				{
					// Make sure a selection isn't created.
					m_iBufferPos_Tail = -1;
				}
				else if (!SelectingText())
				{
					// Place tail at the current point:
					m_iBufferPos_Tail = m_iBufferPos;
				}

				std::list<SRow>::iterator current = m_CharacterPositions.begin();
				while (current != m_CharacterPositions.end())
				{
					if (m_iBufferPos >= current->m_ListStart &&
						m_iBufferPos <= current->m_ListStart+(int)current->m_ListOfX.size())
						break;
					
					++current;
				}

				float pos_x;

				if (m_iBufferPos-current->m_ListStart == 0)
					pos_x = 0.f;
				else
					pos_x = current->m_ListOfX[m_iBufferPos-current->m_ListStart-1];

				if (m_WantedX > pos_x)
					pos_x = m_WantedX;

				// Now change row:
				// Add first, so we can check if it's .end()
				++current;
				if (current != m_CharacterPositions.end())
				{
					// Find X-position:
					m_iBufferPos = current->m_ListStart + GetXTextPosition(current, pos_x, m_WantedX);
				}
				// else we can't move up

				UpdateAutoScroll();
			}
				break;

			case SDLK_PAGEUP:
				GetScrollBar(0).ScrollMinusPlenty();
				break;

			case SDLK_PAGEDOWN:
				GetScrollBar(0).ScrollPlusPlenty();
				break;
			/* END: Message History Lookup */

			case '\r':
				// 'Return' should do a Press event for single liners (e.g. submitting forms)
				//  otherwise a '\n' character will be added.
				{
				bool multiline;
				GUI<bool>::GetSetting(this, "multiline", multiline);
				if (!multiline)
				{
					SendEvent(GUIM_PRESSED, "press");
					break;
				}

				cooked = '\n'; // Change to '\n' and do default:
				// NOTE: Fall-through
				}
			default: //Insert a character
				{
				// If there's a selection, delete if first.
				if (cooked == 0)
					return IN_PASS; // Important, because we didn't use any key

				// check max length
				int max_length;
				GUI<int>::GetSetting(this, "max_length", max_length);
				if (max_length != 0 && (int)pCaption->length() >= max_length)
					break;

				m_WantedX=0.f;

				if (SelectingText())
					DeleteCurSelection();
				m_iBufferPos_Tail = -1;

				if (m_iBufferPos == (int)pCaption->length())
					*pCaption += cooked;
				else
					*pCaption = pCaption->Left(m_iBufferPos) + cooked +
								pCaption->Right((long) pCaption->length()-m_iBufferPos);

				UpdateText(m_iBufferPos, m_iBufferPos, m_iBufferPos+1);

				++m_iBufferPos;

				UpdateAutoScroll();
				}
				break;
		}

		return IN_HANDLED;
	}

	return IN_PASS;
}

void CInput::HandleMessage(SGUIMessage &Message)
{
	// TODO Gee:
	IGUIScrollBarOwner::HandleMessage(Message);

	switch (Message.type)
	{
	case GUIM_SETTINGS_UPDATED:
		{
		bool scrollbar;
		GUI<bool>::GetSetting(this, "scrollbar", scrollbar);

		// Update scroll-bar
		// TODO Gee: (2004-09-01) Is this really updated each time it should?
		if (scrollbar && 
		    (Message.value == CStr("size") || 
			 Message.value == CStr("z") ||
			 Message.value == CStr("absolute")))
		{		
			GetScrollBar(0).SetX(m_CachedActualSize.right);
			GetScrollBar(0).SetY(m_CachedActualSize.top);
			GetScrollBar(0).SetZ(GetBufferedZ());
			GetScrollBar(0).SetLength(m_CachedActualSize.bottom - m_CachedActualSize.top);
		}

		// Update scrollbar
		if (Message.value == CStr("scrollbar_style"))
		{
			CStr scrollbar_style;
			GUI<CStr>::GetSetting(this, Message.value, scrollbar_style);

			GetScrollBar(0).SetScrollBarStyle(scrollbar_style);
		}

		if (Message.value == CStr("size") || 
			Message.value == CStr("z") ||
			Message.value == CStr("font") || 
			Message.value == CStr("absolute") ||
			Message.value == CStr("caption") ||
			Message.value == CStr("scrollbar") ||
			Message.value == CStr("scrollbar_style"))
		{
			UpdateText();
		}

		if (Message.value == CStr("multiline"))
		{
			bool multiline;
			GUI<bool>::GetSetting(this, "multiline", multiline);

			if (multiline == false)
			{
				GetScrollBar(0).SetLength(0.f);
			}
			else
			{
				GetScrollBar(0).SetLength( m_CachedActualSize.bottom - m_CachedActualSize.top );
			}

			UpdateText();
		}
		
		}break;

	case GUIM_MOUSE_PRESS_LEFT:
		// Check if we're selecting the scrollbar:
		{
		bool scrollbar, multiline;
		GUI<bool>::GetSetting(this, "scrollbar", scrollbar);
		GUI<bool>::GetSetting(this, "multiline", multiline);

		if (GetScrollBar(0).GetStyle() && multiline)
		{
			if (GetMousePos().x > m_CachedActualSize.right - GetScrollBar(0).GetStyle()->m_Width)
				break;
		}

		// Okay, this section is about pressing the mouse and
		//  choosing where the point should be placed. For
		//  instance, if we press between a and b, the point
		//  should of course be placed accordingly. Other
		//  special cases are handled like the input box norms.
		if (g_keys[SDLK_RSHIFT] || g_keys[SDLK_LSHIFT])
		{
			m_iBufferPos = GetMouseHoveringTextPosition();
		}
		else
		{
			m_iBufferPos = m_iBufferPos_Tail = GetMouseHoveringTextPosition();
		}

		m_SelectingText = true;
		
		UpdateAutoScroll();

		// If we immediately release the button it will just be seen as a click
		//  for the user though.

		}break;

	case GUIM_MOUSE_RELEASE_LEFT:
		if (m_SelectingText)
		{
			m_SelectingText = false;
		}
		break;
	case GUIM_MOUSE_MOTION:
		// If we just pressed down and started to move before releasing
		//  this is one way of selecting larger portions of text.
		if (m_SelectingText)
		{
			// Actually, first we need to re-check that the mouse button is
			//  really pressed (it can be released while outside the control.
			if (!g_mouse_buttons[SDL_BUTTON_LEFT])
				m_SelectingText = false;
			else
				m_iBufferPos = GetMouseHoveringTextPosition();

			UpdateAutoScroll();
		}

		break;

	case GUIM_MOUSE_WHEEL_DOWN:
		{
		GetScrollBar(0).ScrollPlus();
		// Since the scroll was changed, let's simulate a mouse movement
		//  to check if scrollbar now is hovered
		SGUIMessage msg(GUIM_MOUSE_MOTION);
		HandleMessage(msg);
		break;
		}
	case GUIM_MOUSE_WHEEL_UP:
		{
		GetScrollBar(0).ScrollMinus();
		// Since the scroll was changed, let's simulate a mouse movement
		//  to check if scrollbar now is hovered
		SGUIMessage msg(GUIM_MOUSE_MOTION);
		HandleMessage(msg);
		break;
		}
	case GUIM_LOAD:
		{
		GetScrollBar(0).SetX( m_CachedActualSize.right );
		GetScrollBar(0).SetY( m_CachedActualSize.top );
		GetScrollBar(0).SetZ( GetBufferedZ() );
		GetScrollBar(0).SetLength( m_CachedActualSize.bottom - m_CachedActualSize.top );

		CStr scrollbar_style;
		GUI<CStr>::GetSetting(this, "scrollbar_style", scrollbar_style);
		GetScrollBar(0).SetScrollBarStyle( scrollbar_style );

		UpdateText();
		}
		break;

	case GUIM_GOT_FOCUS:
		m_iBufferPos = 0;
		
		break;

	case GUIM_LOST_FOCUS:
		m_iBufferPos = -1;
		m_iBufferPos_Tail = -1;
		break;

	default:
		break;
	}
}

void CInput::UpdateCachedSize()
{
	// If an ancestor's size changed, this will let us intercept the change and
	// update our scrollbar positions

	IGUIObject::UpdateCachedSize();

	bool scrollbar;
	GUI<bool>::GetSetting(this, "scrollbar", scrollbar);
	if (scrollbar)
	{
		GetScrollBar(0).SetX(m_CachedActualSize.right);
		GetScrollBar(0).SetY(m_CachedActualSize.top);
		GetScrollBar(0).SetZ(GetBufferedZ());
		GetScrollBar(0).SetLength(m_CachedActualSize.bottom - m_CachedActualSize.top);
	}
}

void CInput::Draw()
{
	float bz = GetBufferedZ();

	// First call draw on ScrollBarOwner
	bool scrollbar;
	float buffer_zone;
	bool multiline;
	GUI<bool>::GetSetting(this, "scrollbar", scrollbar);
	GUI<float>::GetSetting(this, "buffer_zone", buffer_zone);
	GUI<bool>::GetSetting(this, "multiline", multiline);

	if (scrollbar && multiline)
	{
		// Draw scrollbar
		IGUIScrollBarOwner::Draw();
	}

	if (GetGUI())
	{	
		CStrW font_name;
		CColor color, color_selected;
		//CStrW caption;
		GUI<CStrW>::GetSetting(this, "font", font_name);
		GUI<CColor>::GetSetting(this, "textcolor", color);
		GUI<CColor>::GetSetting(this, "textcolor_selected", color_selected);
		
		// Get pointer of caption, it might be very large, and we don't
		//  want to copy it continuously.
		CStrW *pCaption = (CStrW*)m_Settings["caption"].m_pSetting;

		CGUISpriteInstance *sprite=NULL, *sprite_selectarea=NULL;
		int cell_id;
		
		GUI<CGUISpriteInstance>::GetSettingPointer(this, "sprite", sprite);
		GUI<CGUISpriteInstance>::GetSettingPointer(this, "sprite_selectarea", sprite_selectarea);
	
		GUI<int>::GetSetting(this, "cell_id", cell_id);

		GetGUI()->DrawSprite(*sprite, cell_id, bz, m_CachedActualSize);

		float scroll=0.f;
		if (scrollbar && multiline)
		{
			scroll = GetScrollBar(0).GetPos();
		}

		glEnable(GL_TEXTURE_2D);
		glDisable(GL_CULL_FACE);

		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_BLEND);
		glDisable(GL_ALPHA_TEST);

		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

		CFont font(font_name);
		font.Bind();

		glPushMatrix();

		// We'll have to setup clipping manually, since we're doing the rendering manually.
		CRect cliparea(m_CachedActualSize);

		// First we'll figure out the clipping area, which is the cached actual size
		//  substracted by an optional scrollbar
		if (scrollbar)
		{
			scroll = GetScrollBar(0).GetPos();

			// substract scrollbar from cliparea
			if (cliparea.right > GetScrollBar(0).GetOuterRect().left &&
				cliparea.right <= GetScrollBar(0).GetOuterRect().right)
				cliparea.right = GetScrollBar(0).GetOuterRect().left;

			if (cliparea.left >= GetScrollBar(0).GetOuterRect().left &&
				cliparea.left < GetScrollBar(0).GetOuterRect().right)
				cliparea.left = GetScrollBar(0).GetOuterRect().right;
		}

		if (cliparea != CRect())
		{
			double eq[4][4] = 
			{ 
				{  0.0,  1.0, 0.0, -cliparea.top },
				{  1.0,  0.0, 0.0, -cliparea.left },
				{  0.0, -1.0, 0.0, cliparea.bottom },
				{ -1.0,  0.0, 0.0, cliparea.right }
			};

			for (int i=0; i<4; ++i)
			{
				glClipPlane(GL_CLIP_PLANE0+i, eq[i]);
				glEnable(GL_CLIP_PLANE0+i);
			}
		}

		// These are useful later.
		int VirtualFrom, VirtualTo;

		if (m_iBufferPos_Tail >= m_iBufferPos)
		{
			VirtualFrom = m_iBufferPos;
			VirtualTo = m_iBufferPos_Tail;
		}
		else
		{
			VirtualFrom = m_iBufferPos_Tail;
			VirtualTo = m_iBufferPos;
		}

		// Get the height of this font.
		float h = (float)font.GetHeight();
		float ls = (float)font.GetLineSpacing();

		// Set the Z to somewhat more, so we can draw a selected area between the
		//  the control and the text.
		glTranslatef((GLfloat)int(m_CachedActualSize.left) + buffer_zone, 
					 (GLfloat)int(m_CachedActualSize.top+h) + buffer_zone, bz+0.1f);
		
		//glColor4f(1.f, 1.f, 1.f, 1.f);
		
		// U+FE33: PRESENTATION FORM FOR VERTICAL LOW LINE
		// (sort of like a | which is aligned to the left of most characters)

		float buffered_y = -scroll+buffer_zone;

		// When selecting larger areas, we need to draw a rectangle box
		//  around it, and this is to keep track of where the box
		//  started, because we need to follow the iteration until we
		//  reach the end, before we can actually draw it.
		bool drawing_box = false;
		float box_x=0.f;

		float x_pointer=0.f;

		// If we have a selecting box (i.e. when you have selected letters, not just when
		//  the pointer is between two letters) we need to process all letters once
		//  before we do it the second time and render all the text. We can't do it
		//  in the same loop because text will have been drawn, so it will disappear when
		//  drawn behind the text that has already been drawn. Confusing, well it's necessary
		//  (I think).

		if (SelectingText())
		{
			// Now m_iBufferPos_Tail can be of both sides of m_iBufferPos,
			//  just like you can select from right to left, as you can
			//  left to right. Is there a difference? Yes, the pointer
			//  be placed accordingly, so that if you select shift and
			//  expand this selection, it will expand on appropriate side.
			// Anyway, since the drawing procedure needs "To" to be
			//  greater than from, we need virtual values that might switch
			//  place.

			int VirtualFrom, VirtualTo;

			if (m_iBufferPos_Tail >= m_iBufferPos)
			{
				VirtualFrom = m_iBufferPos;
				VirtualTo = m_iBufferPos_Tail;
			}
			else
			{
				VirtualFrom = m_iBufferPos_Tail;
				VirtualTo = m_iBufferPos;
			}


			bool done = false;
			for (std::list<SRow>::const_iterator it = m_CharacterPositions.begin();
				it != m_CharacterPositions.end();
				++it, buffered_y += ls, x_pointer = 0.f)
			{
				if (multiline)
				{
					if (buffered_y > m_CachedActualSize.GetHeight())
						break;
				}

				// We might as well use 'i' here to iterate, because we need it
				// (often compared against ints, so don't make it size_t)
				for (int i=0; i < (int)it->m_ListOfX.size()+2; ++i)
				{
					if (it->m_ListStart + i == VirtualFrom)
					{
						// we won't actually draw it now, because we don't
						//  know the width of each glyph to that position. 
						//  we need to go along with the iteration, and
						//  make a mark where the box started:
						drawing_box = true; // will turn false when finally rendered.

						// Get current x position
						box_x = x_pointer;
					}

					// no else!

					const bool at_end = (i == (int)it->m_ListOfX.size()+1);

					if (drawing_box == true &&
						(it->m_ListStart + i == VirtualTo || at_end))
					{
						// Depending on if it's just a row change, or if it's
						//  the end of the select box, do slightly different things.
						if (at_end)
						{
							if (it->m_ListStart + i != VirtualFrom)
							{
								// and actually add a white space! yes, this is done in any common input
								x_pointer += (float)font.GetCharacterWidth(L' ');
							}
						}
						else
						{
							drawing_box = false;
							done = true;
						}

						CRect rect;
						// Set 'rect' depending on if it's a multiline control, or a one-line control
						if (multiline)
						{
							rect = CRect(m_CachedActualSize.left+box_x+buffer_zone, 
									   m_CachedActualSize.top+buffered_y+(h-ls)/2,
									   m_CachedActualSize.left+x_pointer+buffer_zone, 
									   m_CachedActualSize.top+buffered_y+(h+ls)/2);

							if (rect.bottom < m_CachedActualSize.top)
								continue;

							if (rect.top < m_CachedActualSize.top)
								rect.top = m_CachedActualSize.top;

							if (rect.bottom > m_CachedActualSize.bottom)
								rect.bottom = m_CachedActualSize.bottom;
						}
						else // if one-line
						{
							rect = CRect(m_CachedActualSize.left+box_x+buffer_zone-m_HorizontalScroll, 
									   m_CachedActualSize.top+buffered_y+(h-ls)/2,
									   m_CachedActualSize.left+x_pointer+buffer_zone-m_HorizontalScroll, 
									   m_CachedActualSize.top+buffered_y+(h+ls)/2);

							if (rect.left < m_CachedActualSize.left)
								rect.left = m_CachedActualSize.left;

							if (rect.right > m_CachedActualSize.right)
								rect.right = m_CachedActualSize.right;
						}

						glPushMatrix();
						guiLoadIdentity();
						glEnable(GL_BLEND);
						glDisable(GL_TEXTURE_2D);

						if (sprite_selectarea)
							GetGUI()->DrawSprite(*sprite_selectarea, cell_id, bz+0.05f, rect);

						// Blend can have been reset
						glEnable(GL_BLEND);
						glEnable(GL_TEXTURE_2D);
						glDisable(GL_ALPHA_TEST);

						glPopMatrix();
					}

					if (i < (int)it->m_ListOfX.size())
                        x_pointer += (float)font.GetCharacterWidth((*pCaption)[it->m_ListStart + i]);
				}

				if (done)
					break;

				// If we're about to draw a box, and all of a sudden changes
				//  line, we need to draw that line's box, and then reset
				//  the box drawing to the beginning of the new line.
				if (drawing_box)
				{
					box_x = 0.f;
				}
			}
		}

		// Reset some from previous run
		buffered_y = -scroll;
		
		// Setup initial color (then it might change and change back, when drawing selected area)
		glColor4f(color.r, color.g, color.b, color.a);

		bool using_selected_color = false;
		
		for (std::list<SRow>::const_iterator it = m_CharacterPositions.begin();
			 it != m_CharacterPositions.end();
			 ++it, buffered_y += ls)
		{
			if (buffered_y + buffer_zone >= -ls || !multiline)
			{
				if (multiline)
				{
                    if (buffered_y + buffer_zone > m_CachedActualSize.GetHeight())
						break;
				}

				glPushMatrix();
				
				// Text must always be drawn in integer values. So we have to convert scroll
				if (multiline)
                    glTranslatef(0.f, -(float)(int)scroll, 0.f);
				else
					glTranslatef(-(float)(int)m_HorizontalScroll, 0.f, 0.f);

				// We might as well use 'i' here, because we need it
				// (often compared against ints, so don't make it size_t)
				for (int i=0; i < (int)it->m_ListOfX.size()+1; ++i)
				{
					if (!multiline && i < (int)it->m_ListOfX.size())
					{
						if (it->m_ListOfX[i] - m_HorizontalScroll < -buffer_zone)
						{
							// We still need to translate the OpenGL matrix
							if (i == 0)
								glTranslatef(it->m_ListOfX[i], 0.f, 0.f);
							else
								glTranslatef(it->m_ListOfX[i] - it->m_ListOfX[i-1], 0.f, 0.f);

							continue;
						}
					}

					// End of selected area, change back color
					if (SelectingText() && 
						it->m_ListStart + i == VirtualTo)
					{
						using_selected_color = false;
						glColor4f(color.r, color.g, color.b, color.a);
					}

					if (i != (int)it->m_ListOfX.size() &&
						it->m_ListStart + i == m_iBufferPos)
					{
						// selecting only one, then we need only to draw a cursor.
						glPushMatrix();
						glwprintf(L"_");
						glPopMatrix();
					}

					// Drawing selected area
					if (SelectingText() && 
						it->m_ListStart + i >= VirtualFrom &&
						it->m_ListStart + i < VirtualTo &&
						using_selected_color == false)
					{
						using_selected_color = true;
						glColor4f(color_selected.r, color_selected.g, color_selected.b, color_selected.a);
					}

					if (i != (int)it->m_ListOfX.size())
						glwprintf(L"%lc", (*pCaption)[it->m_ListStart + i]);

					// check it's now outside a one-liner, then we'll break
					if (!multiline && i < (int)it->m_ListOfX.size())				
					{
						if (it->m_ListOfX[i] - m_HorizontalScroll > m_CachedActualSize.GetWidth()-buffer_zone)
							break;
					}
				}

				if (it->m_ListStart + (int)it->m_ListOfX.size() == m_iBufferPos)
				{
					glColor4f(color.r, color.g, color.b, color.a);
					glwprintf(L"_");

					if (using_selected_color)
					{
						glColor4f(color_selected.r, color_selected.g, color_selected.b, color_selected.a);
					}
				}

				glPopMatrix();
			}
			glTranslatef(0.f, ls, 0.f);	
		}

		glPopMatrix();

		// Disable clipping
		for (int i=0; i<4; ++i)
			glDisable(GL_CLIP_PLANE0+i);
		glDisable(GL_TEXTURE_2D);
	}
}

void CInput::UpdateText(int from, int to_before, int to_after)
{
	CStrW caption;
	CStrW font_name;
	float buffer_zone;
	bool multiline;
	GUI<CStrW>::GetSetting(this, "font", font_name);
	GUI<CStrW>::GetSetting(this, "caption", caption);
	GUI<float>::GetSetting(this, "buffer_zone", buffer_zone);
	GUI<bool>::GetSetting(this, "multiline", multiline);

	// Ensure positions are valid after caption changes
	m_iBufferPos = std::min(m_iBufferPos, (int)caption.size());
	m_iBufferPos_Tail = std::min(m_iBufferPos_Tail, (int)caption.size());

	if (font_name == CStrW())
	{
		// Destroy everything stored, there's no font, so there can be
		//  no data.
		m_CharacterPositions.clear();
		return;
	}

	SRow row;
	row.m_ListStart = 0;
	
	int to = 0;	// make sure it's initialized

	if (to_before == -1)
		to = (int)caption.length();

	CFont font(font_name);

	std::list<SRow>::iterator current_line;

	// Used to ... TODO
	int check_point_row_start = -1;
	int check_point_row_end = -1;

	// Reset
	if (from == 0 && to_before == -1)
	{
		m_CharacterPositions.clear();
		current_line = m_CharacterPositions.begin();
	}
	else
	{
		ENSURE(to_before != -1);

		std::list<SRow>::iterator destroy_row_from, destroy_row_to;
		// Used to check if the above has been set to anything, 
		//  previously a comparison like:
		//  destroy_row_from == std::list<SRow>::iterator()
		// ... was used, but it didn't work with GCC.
		bool destroy_row_from_used=false, destroy_row_to_used=false;

		// Iterate, and remove everything between 'from' and 'to_before'
		//  actually remove the entire lines they are on, it'll all have
		//  to be redone. And when going along, we'll delete a row at a time
		//  when continuing to see how much more after 'to' we need to remake.
		int i=0;
		for (std::list<SRow>::iterator it=m_CharacterPositions.begin(); 
			 it!=m_CharacterPositions.end(); ++it, ++i)
		{
			if (destroy_row_from_used == false &&
				it->m_ListStart > from)
			{
				// Destroy the previous line, and all to 'to_before'
				destroy_row_from = it;
				--destroy_row_from;

				destroy_row_from_used = true;

				// For the rare case that we might remove characters to a word
				//  so that it suddenly fits on the previous row,
				//  we need to by standards re-do the whole previous line too 
				//  (if one exists)
				if (destroy_row_from != m_CharacterPositions.begin())
					--destroy_row_from;
			}

			if (destroy_row_to_used == false &&
				it->m_ListStart > to_before)
			{
				destroy_row_to = it;

				destroy_row_to_used = true;

				// If it isn't the last row, we'll add another row to delete,
				//  just so we can see if the last restorted line is 
				//  identical to what it was before. If it isn't, then we'll
				//  have to continue.
				// 'check_point_row_start' is where we store how the that
				//  line looked.
				if (destroy_row_to != m_CharacterPositions.end())
				{
					check_point_row_start = destroy_row_to->m_ListStart;
					check_point_row_end = check_point_row_start + (int)destroy_row_to->m_ListOfX.size();
					if (destroy_row_to->m_ListOfX.empty())
						++check_point_row_end;
				}

				++destroy_row_to;
				break;
			}
		}

		if (destroy_row_from_used == false)
		{
			destroy_row_from = m_CharacterPositions.end();
			--destroy_row_from;

			// As usual, let's destroy another row back
			if (destroy_row_from != m_CharacterPositions.begin())
				--destroy_row_from;

			destroy_row_from_used = true;

			current_line = destroy_row_from;
		}

		if (destroy_row_to_used == false)
		{
            destroy_row_to = m_CharacterPositions.end();
			check_point_row_start = -1;

			destroy_row_from_used = true;
		}

		// set 'from' to the row we'll destroy from
		//  and 'to' to the row we'll destroy to
		from = destroy_row_from->m_ListStart;
		
		if (destroy_row_to != m_CharacterPositions.end())
            to = destroy_row_to->m_ListStart; // notice it will iterate [from, to), so it will never reach to.
		else
			to = (int)caption.length();


		// Setup the first row
		row.m_ListStart = destroy_row_from->m_ListStart;

		// Set current line, new rows will be added before current_line, so
		//  we'll choose the destroy_row_to, because it won't be deleted
		//  in the coming erase.
		current_line = destroy_row_to;

		std::list<SRow>::iterator temp_it = destroy_row_to;
		--temp_it;

		m_CharacterPositions.erase(destroy_row_from, destroy_row_to);
		
		// If there has been a change in number of characters
		//  we need to change all m_ListStart that comes after
		//  the interval we just destroyed. We'll change all
		//  values with the delta change of the string length.
		int delta = to_after - to_before;
		if (delta != 0)
		{
			for (std::list<SRow>::iterator it=current_line;
				 it!=m_CharacterPositions.end();
				 ++it)
			{
				it->m_ListStart += delta;
			}

			// Update our check point too!
			check_point_row_start += delta;
			check_point_row_end += delta;

			if (to != (int)caption.length())
				to += delta;
		}
	}
	
	int last_word_started=from;
	//int last_list_start=-1;	// unused
	float x_pos = 0.f;

	//if (to_before != -1)
	//	return;

	for (int i=from; i<to; ++i)
	{
		if (caption[i] == L'\n' && multiline)
		{
			if (i==to-1 && to != (int)caption.length())
				break; // it will be added outside
			
			current_line = m_CharacterPositions.insert( current_line, row );
			++current_line;


			// Setup the next row:
			row.m_ListOfX.clear();
			row.m_ListStart = i+1;
			x_pos = 0.f;

		}
		else
		{
			if (caption[i] == L' '/* || TODO Gee (2004-10-13): the '-' disappears, fix.
				caption[i] == L'-'*/)
				last_word_started = i+1;

			x_pos += (float)font.GetCharacterWidth(caption[i]);

			if (x_pos >= GetTextAreaWidth() && multiline)
			{
				// The following decides whether it will word-wrap a word,
				//  or if it's only one word on the line, where it has to
				//  break the word apart.
				if (last_word_started == row.m_ListStart)
				{
					last_word_started = i;
					row.m_ListOfX.resize(row.m_ListOfX.size() - (i-last_word_started));
					//row.m_ListOfX.push_back( x_pos );
					//continue;
				}
				else
				{
					// regular word-wrap
					row.m_ListOfX.resize(row.m_ListOfX.size() - (i-last_word_started+1));
				}
				
				// Now, create a new line:
				//  notice: when we enter a newline, you can stand with the cursor
				//  both before and after that character, being on different
				//  rows. With automatic word-wrapping, that is not possible. Which
				//  is intuitively correct.

				current_line = m_CharacterPositions.insert( current_line, row );
				++current_line;

				// Setup the next row:
				row.m_ListOfX.clear();
				row.m_ListStart = last_word_started;

				i=last_word_started-1;

				x_pos = 0.f;
			}
			else
				// Get width of this character:
				row.m_ListOfX.push_back( x_pos );
		}

		// Check if it's the last iteration, and we're not revising the whole string
		//  because in that case, more word-wrapping might be needed.
		//  also check if the current line isn't the end
		if (to_before != -1 && i == to-1 && current_line != m_CharacterPositions.end())
		{
			// check all rows and see if any existing 
			if (row.m_ListStart != check_point_row_start)
			{
				std::list<SRow>::iterator destroy_row_from, destroy_row_to;
				// Are used to check if the above has been set to anything, 
				//  previously a comparison like:
				//  destroy_row_from == std::list<SRow>::iterator()
				//  was used, but it didn't work with GCC.
				bool destroy_row_from_used=false, destroy_row_to_used=false;

				// Iterate, and remove everything between 'from' and 'to_before'
				//  actually remove the entire lines they are on, it'll all have
				//  to be redone. And when going along, we'll delete a row at a time
				//  when continuing to see how much more after 'to' we need to remake.
				int i=0;
				for (std::list<SRow>::iterator it=m_CharacterPositions.begin(); 
					it!=m_CharacterPositions.end(); ++it, ++i)
				{
					if (destroy_row_from_used == false &&
						it->m_ListStart > check_point_row_start)
					{
						// Destroy the previous line, and all to 'to_before'
						//if (i >= 2)
						//	destroy_row_from = it-2;
						//else
						//	destroy_row_from = it-1;
						destroy_row_from = it;
						destroy_row_from_used = true;
						//--destroy_row_from;
					}

					if (destroy_row_to_used == false &&
						it->m_ListStart > check_point_row_end)
					{
						destroy_row_to = it;
						destroy_row_to_used = true;

						// If it isn't the last row, we'll add another row to delete,
						//  just so we can see if the last restorted line is 
						//  identical to what it was before. If it isn't, then we'll
						//  have to continue.
						// 'check_point_row_start' is where we store how the that
						//  line looked.
			//			if (destroy_row_to != 
						if (destroy_row_to != m_CharacterPositions.end())
						{
							check_point_row_start = destroy_row_to->m_ListStart;
							check_point_row_end = check_point_row_start + (int)destroy_row_to->m_ListOfX.size();
							if (destroy_row_to->m_ListOfX.empty())
								++check_point_row_end;
						}
						else
							check_point_row_start = check_point_row_end = -1;

						++destroy_row_to;
						break;
					}
				}

				if (destroy_row_from_used == false)
				{
					destroy_row_from = m_CharacterPositions.end();
					--destroy_row_from;

					destroy_row_from_used = true;

					current_line = destroy_row_from;
				}

				if (destroy_row_to_used == false)
				{
					destroy_row_to = m_CharacterPositions.end();
					check_point_row_start = check_point_row_end = -1;

					destroy_row_to_used = true;
				}

				// set 'from' to the from row we'll destroy
				//  and 'to' to 'to_after'
				from = destroy_row_from->m_ListStart;
				
				if (destroy_row_to != m_CharacterPositions.end())
					to = destroy_row_to->m_ListStart; // notice it will iterate [from, to[, so it will never reach to.
				else
					to = (int)caption.length();


				// Set current line, new rows will be added before current_line, so
				//  we'll choose the destroy_row_to, because it won't be deleted
				//  in the coming erase.
				current_line = destroy_row_to;

				std::list<SRow>::iterator temp = destroy_row_to;

				--temp;

				m_CharacterPositions.erase(destroy_row_from, destroy_row_to);
			}
			// else, the for loop will end naturally.
		}
	}
	// This is kind of special, when we renew a some lines, then the last
	//  one will sometimes end with a space (' '), that really should
	//  be omitted when word-wrapping. So we'll check if the last row
	//  we'll add has got the same value as the next row.
	if (current_line != m_CharacterPositions.end())
	{
		if (row.m_ListStart + (int)row.m_ListOfX.size() == current_line->m_ListStart)
			row.m_ListOfX.resize( row.m_ListOfX.size()-1 );
	}

	// add the final row (even if empty)
	m_CharacterPositions.insert(current_line, row);

	bool scrollbar;
	GUI<bool>::GetSetting(this, "scrollbar", scrollbar);
	// Update scollbar
	if (scrollbar)
	{
		GetScrollBar(0).SetScrollRange(m_CharacterPositions.size() * font.GetLineSpacing() + buffer_zone*2.f);
		GetScrollBar(0).SetScrollSpace(m_CachedActualSize.GetHeight());
	}
}

int CInput::GetMouseHoveringTextPosition()
{
	if (m_CharacterPositions.empty())
		return 0;

	// Return position
	int RetPosition;

	float buffer_zone;
	bool multiline;
	GUI<float>::GetSetting(this, "buffer_zone", buffer_zone);
	GUI<bool>::GetSetting(this, "multiline", multiline);

	std::list<SRow>::iterator current = m_CharacterPositions.begin();

	CPos mouse = GetMousePos();

	if (multiline)
	{
		CStrW font_name;
		bool scrollbar;
		GUI<CStrW>::GetSetting(this, "font", font_name);
		GUI<bool>::GetSetting(this, "scrollbar", scrollbar);
		
		float scroll=0.f;
		if (scrollbar)
		{
			scroll = GetScrollBar(0).GetPos();
		}

		// Pointer to caption, will come in handy
		CStrW *pCaption = (CStrW*)m_Settings["caption"].m_pSetting;
		UNUSED2(pCaption);

		// Now get the height of the font.
						// TODO: Get the real font
		CFont font(font_name);
		float spacing = (float)font.GetLineSpacing();
		//float height = (float)font.GetHeight();	// unused

		// Change mouse position relative to text.
		mouse -= m_CachedActualSize.TopLeft();
		mouse.x -= buffer_zone;
		mouse.y += scroll - buffer_zone;

		//if ((m_CharacterPositions.size()-1) * spacing + height < mouse.y)
		//	m_iBufferPos = pCaption->Length();
		int row = (int)((mouse.y) / spacing);//m_CharachterPositions.size()

		if (row < 0)
			row = 0;

		if (row > (int)m_CharacterPositions.size()-1)
			row = (int)m_CharacterPositions.size()-1;

		// TODO Gee (2004-11-21): Okay, I need a 'std::list' for some reasons, but I would really like to
		//  be able to get the specific element here. This is hopefully a temporary hack.

		for (int i=0; i<row; ++i)
			++current;
	}
	else
	{
		// current is already set to begin,
		//  but we'll change the mouse.x to fit our horizontal scrolling
		mouse -= m_CachedActualSize.TopLeft();
		mouse.x -= buffer_zone - m_HorizontalScroll;
		// mouse.y is moot
	}

	//m_iBufferPos = m_CharacterPositions.get.m_ListStart;
	RetPosition = current->m_ListStart;
	
	// Okay, now loop through the glyphs to find the appropriate X position
	float dummy;
	RetPosition += GetXTextPosition(current, mouse.x, dummy);

	return RetPosition;
}

// Does not process horizontal scrolling, 'x' must be modified before inputted.
int CInput::GetXTextPosition(const std::list<SRow>::iterator &current, const float &x, float &wanted)
{
	int Ret=0;

	float previous=0.f;
	int i=0;

	for (std::vector<float>::iterator it=current->m_ListOfX.begin();
			it!=current->m_ListOfX.end();
			++it, ++i)
	{
		if (*it >= x)
		{
			if (x - previous >= *it - x)
				Ret += i+1;
			else
				Ret += i;

			break;
		}
		previous = *it;
	}
	// If a position wasn't found, we will assume the last
	//  character of that line.
	if (i == (int)current->m_ListOfX.size())
	{
		Ret += i;
		wanted = x;
	}
	else wanted = 0.f;

	return Ret;
}

void CInput::DeleteCurSelection()
{
	CStrW *pCaption = (CStrW*)m_Settings["caption"].m_pSetting;

	int VirtualFrom, VirtualTo;

	if (m_iBufferPos_Tail >= m_iBufferPos)
	{
		VirtualFrom = m_iBufferPos;
		VirtualTo = m_iBufferPos_Tail;
	}
	else
	{
		VirtualFrom = m_iBufferPos_Tail;
		VirtualTo = m_iBufferPos;
	}

	*pCaption = pCaption->Left( VirtualFrom ) + 
				pCaption->Right( (long) pCaption->length()-(VirtualTo) );

	UpdateText(VirtualFrom, VirtualTo, VirtualFrom);

	// Remove selection
	m_iBufferPos_Tail = -1;
	m_iBufferPos = VirtualFrom;
}

bool CInput::SelectingText() const
{
	return m_iBufferPos_Tail != -1 &&
		   m_iBufferPos_Tail != m_iBufferPos;
}

float CInput::GetTextAreaWidth()
{
	bool scrollbar;
	float buffer_zone;
	GUI<bool>::GetSetting(this, "scrollbar", scrollbar);
	GUI<float>::GetSetting(this, "buffer_zone", buffer_zone);

	if (scrollbar && GetScrollBar(0).GetStyle())
		return m_CachedActualSize.GetWidth() - buffer_zone*2.f - GetScrollBar(0).GetStyle()->m_Width;
	else
		return m_CachedActualSize.GetWidth() - buffer_zone*2.f;
}

void CInput::UpdateAutoScroll()
{
	float buffer_zone;
	bool multiline;
	GUI<float>::GetSetting(this, "buffer_zone", buffer_zone);
	GUI<bool>::GetSetting(this, "multiline", multiline);

	// Autoscrolling up and down
	if (multiline)
	{
		CStrW font_name;
		bool scrollbar;
		GUI<CStrW>::GetSetting(this, "font", font_name);
		GUI<bool>::GetSetting(this, "scrollbar", scrollbar);
		
		float scroll=0.f;
		if (!scrollbar)
			return;

		scroll = GetScrollBar(0).GetPos();
		
		// Now get the height of the font.
						// TODO: Get the real font
		CFont font(font_name);
		float spacing = (float)font.GetLineSpacing();
		//float height = font.GetHeight();

		// TODO Gee (2004-11-21): Okay, I need a 'std::list' for some reasons, but I would really like to
		//  be able to get the specific element here. This is hopefully a temporary hack.

		std::list<SRow>::iterator current = m_CharacterPositions.begin();
		int row=0;
		while (current != m_CharacterPositions.end())
		{
			if (m_iBufferPos >= current->m_ListStart &&
				m_iBufferPos <= current->m_ListStart+(int)current->m_ListOfX.size())
				break;
			
			++current;
			++row;
		}

		// If scrolling down
		if (-scroll + (float)(row+1) * spacing + buffer_zone*2.f > m_CachedActualSize.GetHeight())
		{
			// Scroll so the selected row is shown completely, also with buffer_zone length to the edge.
			GetScrollBar(0).SetPos((float)(row+1) * spacing - m_CachedActualSize.GetHeight() + buffer_zone*2.f);
		}
		else
		// If scrolling up
		if (-scroll + (float)row * spacing < 0.f)
		{
			// Scroll so the selected row is shown completely, also with buffer_zone length to the edge.
			GetScrollBar(0).SetPos((float)row * spacing);
		}
	}
	else // autoscrolling left and right
	{
		// Get X position of position:
		if (m_CharacterPositions.empty())
			return;

		float x_position = 0.f;
		float x_total = 0.f;
		if (!m_CharacterPositions.begin()->m_ListOfX.empty())
		{

			// Get position of m_iBufferPos
			if ((int)m_CharacterPositions.begin()->m_ListOfX.size() >= m_iBufferPos &&
				m_iBufferPos != 0)
				x_position = m_CharacterPositions.begin()->m_ListOfX[m_iBufferPos-1];

			// Get complete length:
			x_total = m_CharacterPositions.begin()->m_ListOfX[ m_CharacterPositions.begin()->m_ListOfX.size()-1 ];
		}

		// Check if outside to the right
		if (x_position - m_HorizontalScroll + buffer_zone*2.f > m_CachedActualSize.GetWidth())
			m_HorizontalScroll = x_position - m_CachedActualSize.GetWidth() + buffer_zone*2.f;

		// Check if outside to the left
		if (x_position - m_HorizontalScroll < 0.f)
			m_HorizontalScroll = x_position;

		// Check if the text doesn't even fill up to the right edge even though scrolling is done.
		if (m_HorizontalScroll != 0.f &&
			x_total - m_HorizontalScroll + buffer_zone*2.f < m_CachedActualSize.GetWidth())
			m_HorizontalScroll = x_total - m_CachedActualSize.GetWidth() + buffer_zone*2.f;

		// Now this is the fail-safe, if x_total isn't even the length of the control,
		//  remove all scrolling
		if (x_total + buffer_zone*2.f < m_CachedActualSize.GetWidth())
			m_HorizontalScroll = 0.f;
	}
}
