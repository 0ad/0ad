/* Copyright (C) 2017 Wildfire Games.
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

#include "CInput.h"

#include "CGUIScrollBarVertical.h"
#include "GUI.h"

#include "graphics/FontMetrics.h"
#include "graphics/ShaderManager.h"
#include "graphics/TextRenderer.h"
#include "lib/ogl.h"
#include "lib/sysdep/clipboard.h"
#include "lib/timer.h"
#include "lib/utf8.h"
#include "ps/CLogger.h"
#include "ps/ConfigDB.h"
#include "ps/GameSetup/Config.h"
#include "ps/Globals.h"
#include "ps/Hotkey.h"
#include "renderer/Renderer.h"

#include <sstream>

extern int g_yres;

CInput::CInput()
	: m_iBufferPos(-1), m_iBufferPos_Tail(-1), m_SelectingText(false), m_HorizontalScroll(0.f),
	m_PrevTime(0.0), m_CursorVisState(true), m_CursorBlinkRate(0.5), m_ComposingText(false),
	m_iComposedLength(0), m_iComposedPos(0), m_iInsertPos(0), m_Readonly(false)
{
	AddSetting(GUIST_int,					"buffer_position");
	AddSetting(GUIST_float,					"buffer_zone");
	AddSetting(GUIST_CStrW,					"caption");
	AddSetting(GUIST_int,					"cell_id");
	AddSetting(GUIST_CStrW,					"font");
	AddSetting(GUIST_CStrW,					"mask_char");
	AddSetting(GUIST_bool,					"mask");
	AddSetting(GUIST_int,					"max_length");
	AddSetting(GUIST_bool,					"multiline");
	AddSetting(GUIST_bool,					"readonly");
	AddSetting(GUIST_bool,					"scrollbar");
	AddSetting(GUIST_CStr,					"scrollbar_style");
	AddSetting(GUIST_CGUISpriteInstance,	"sprite");
	AddSetting(GUIST_CGUISpriteInstance,	"sprite_selectarea");
	AddSetting(GUIST_CColor,				"textcolor");
	AddSetting(GUIST_CColor,				"textcolor_selected");
	AddSetting(GUIST_CStrW,					"tooltip");
	AddSetting(GUIST_CStr,					"tooltip_style");

	CFG_GET_VAL("gui.cursorblinkrate", m_CursorBlinkRate);

	CGUIScrollBarVertical* bar = new CGUIScrollBarVertical();
	bar->SetRightAligned(true);
	AddScrollBar(bar);
}

CInput::~CInput()
{
}

void CInput::UpdateBufferPositionSetting()
{
	int* bufferPos = (int*)m_Settings["buffer_position"].m_pSetting;
	*bufferPos = m_iBufferPos;
}

void CInput::ClearComposedText()
{
	CStrW* pCaption = (CStrW*)m_Settings["caption"].m_pSetting;
	pCaption->erase(m_iInsertPos, m_iComposedLength);
	m_iBufferPos = m_iInsertPos;
	UpdateBufferPositionSetting();
	m_iComposedLength = 0;
	m_iComposedPos = 0;
}

InReaction CInput::ManuallyHandleEvent(const SDL_Event_* ev)
{
	ENSURE(m_iBufferPos != -1);

	switch (ev->ev.type)
	{
	case SDL_HOTKEYDOWN:
	{
		if (m_ComposingText)
			return IN_HANDLED;

		return ManuallyHandleHotkeyEvent(ev);
	}
	// SDL2 has a new method of text input that better supports Unicode and CJK
	// see https://wiki.libsdl.org/Tutorials/TextInput
	case SDL_TEXTINPUT:
	{
		if (m_Readonly)
			return IN_PASS;

		// Text has been committed, either single key presses or through an IME
		CStrW* pCaption = (CStrW*)m_Settings["caption"].m_pSetting;
		std::wstring text = wstring_from_utf8(ev->ev.text.text);

		m_WantedX = 0.0f;

		if (SelectingText())
			DeleteCurSelection();

		if (m_ComposingText)
		{
			ClearComposedText();
			m_ComposingText = false;
		}

		if (m_iBufferPos == (int)pCaption->length())
			pCaption->append(text);
		else
			pCaption->insert(m_iBufferPos, text);

		UpdateText(m_iBufferPos, m_iBufferPos, m_iBufferPos+1);

		m_iBufferPos += text.length();
		UpdateBufferPositionSetting();
		m_iBufferPos_Tail = -1;

		UpdateAutoScroll();
		SendEvent(GUIM_TEXTEDIT, "textedit");

		return IN_HANDLED;
	}
	case SDL_TEXTEDITING:
	{
		if (m_Readonly)
			return IN_PASS;

		// Text is being composed with an IME
		// TODO: indicate this by e.g. underlining the uncommitted text
		CStrW* pCaption = (CStrW*)m_Settings["caption"].m_pSetting;
		const char* rawText = ev->ev.edit.text;
		int rawLength = strlen(rawText);
		std::wstring wtext = wstring_from_utf8(rawText);

		debug_printf("SDL_TEXTEDITING: text=%s, start=%d, length=%d\n", rawText, ev->ev.edit.start, ev->ev.edit.length);
		m_WantedX = 0.0f;

		if (SelectingText())
			DeleteCurSelection();

		// Remember cursor position when text composition begins
		if (!m_ComposingText)
			m_iInsertPos = m_iBufferPos;
		else
		{
			// Composed text is replaced each time
			ClearComposedText();
		}

		m_ComposingText = ev->ev.edit.start != 0 || rawLength != 0;
		if (m_ComposingText)
		{
			pCaption->insert(m_iInsertPos, wtext);

			// The text buffer is limited to SDL_TEXTEDITINGEVENT_TEXT_SIZE bytes, yet start
			// increases without limit, so don't let it advance beyond the composed text length
			m_iComposedLength = wtext.length();
			m_iComposedPos = ev->ev.edit.start < m_iComposedLength ? ev->ev.edit.start : m_iComposedLength;
			m_iBufferPos = m_iInsertPos + m_iComposedPos;

			// TODO: composed text selection - what does ev.edit.length do?
			m_iBufferPos_Tail = -1;
		}

		UpdateBufferPositionSetting();
		UpdateText(m_iBufferPos, m_iBufferPos, m_iBufferPos+1);

		UpdateAutoScroll();
		SendEvent(GUIM_TEXTEDIT, "textedit");

		return IN_HANDLED;
	}
	case SDL_KEYDOWN:
	{
		if (m_ComposingText)
			return IN_HANDLED;

		// Since the GUI framework doesn't handle to set settings
		//  in Unicode (CStrW), we'll simply retrieve the actual
		//  pointer and edit that.
		CStrW* pCaption = (CStrW*)m_Settings["caption"].m_pSetting;
		SDL_Keycode keyCode = ev->ev.key.keysym.sym;

		ManuallyImmutableHandleKeyDownEvent(keyCode, pCaption);
		ManuallyMutableHandleKeyDownEvent(keyCode, pCaption);

		UpdateBufferPositionSetting();
		return IN_HANDLED;
	}
	default:
	{
		return IN_PASS;
	}
	}
}

void CInput::ManuallyMutableHandleKeyDownEvent(const SDL_Keycode keyCode, CStrW* pCaption)
{
	if (m_Readonly)
		return;

	wchar_t cooked = 0;

	switch (keyCode)
	{
	case SDLK_TAB:
	{
		SendEvent(GUIM_TAB, "tab");
		// Don't send a textedit event, because it should only
		// be sent if the GUI control changes the text
		break;
	}
	case SDLK_BACKSPACE:
	{
		m_WantedX = 0.0f;

		if (SelectingText())
			DeleteCurSelection();
		else
		{
			m_iBufferPos_Tail = -1;

			if (pCaption->empty() || m_iBufferPos == 0)
				break;

			if (m_iBufferPos == (int)pCaption->length())
				*pCaption = pCaption->Left((long)pCaption->length() - 1);
			else
				*pCaption = pCaption->Left(m_iBufferPos - 1) +
				pCaption->Right((long)pCaption->length() - m_iBufferPos);

			--m_iBufferPos;

			UpdateText(m_iBufferPos, m_iBufferPos + 1, m_iBufferPos);
		}

		UpdateAutoScroll();
		SendEvent(GUIM_TEXTEDIT, "textedit");
		break;
	}
	case SDLK_DELETE:
	{
		m_WantedX = 0.0f;

		if (SelectingText())
			DeleteCurSelection();
		else
		{
			if (pCaption->empty() || m_iBufferPos == (int)pCaption->length())
				break;

			*pCaption = pCaption->Left(m_iBufferPos) +
				pCaption->Right((long)pCaption->length() - (m_iBufferPos + 1));

			UpdateText(m_iBufferPos, m_iBufferPos + 1, m_iBufferPos);
		}

		UpdateAutoScroll();
		SendEvent(GUIM_TEXTEDIT, "textedit");
		break;
	}
	case SDLK_KP_ENTER:
	case SDLK_RETURN:
	{
		// 'Return' should do a Press event for single liners (e.g. submitting forms)
		//  otherwise a '\n' character will be added.
		bool multiline;
		GUI<bool>::GetSetting(this, "multiline", multiline);
		if (!multiline)
		{
			SendEvent(GUIM_PRESSED, "press");
			break;
		}

		cooked = '\n'; // Change to '\n' and do default:
		FALLTHROUGH;
	}
	default: // Insert a character
	{
		// In SDL2, we no longer get Unicode wchars via SDL_Keysym
		// we use text input events instead and they provide UTF-8 chars
		if (cooked == 0)
			return;

		// check max length
		int max_length;
		GUI<int>::GetSetting(this, "max_length", max_length);
		if (max_length != 0 && (int)pCaption->length() >= max_length)
			break;

		m_WantedX = 0.0f;

		if (SelectingText())
			DeleteCurSelection();
		m_iBufferPos_Tail = -1;

		if (m_iBufferPos == (int)pCaption->length())
			*pCaption += cooked;
		else
			*pCaption = pCaption->Left(m_iBufferPos) + cooked +
			pCaption->Right((long)pCaption->length() - m_iBufferPos);

		UpdateText(m_iBufferPos, m_iBufferPos, m_iBufferPos + 1);

		++m_iBufferPos;

		UpdateAutoScroll();
		SendEvent(GUIM_TEXTEDIT, "textedit");
		break;
	}
	}
}

void CInput::ManuallyImmutableHandleKeyDownEvent(const SDL_Keycode keyCode, CStrW* pCaption)
{
	bool shiftKeyPressed = g_keys[SDLK_RSHIFT] || g_keys[SDLK_LSHIFT];

	switch (keyCode)
	{
	case SDLK_HOME:
	{
		// If there's not a selection, we should create one now
		if (!shiftKeyPressed)
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
		m_WantedX = 0.0f;

		UpdateAutoScroll();
		break;
	}
	case SDLK_END:
	{
		// If there's not a selection, we should create one now
		if (!shiftKeyPressed)
		{
			// Make sure a selection isn't created.
			m_iBufferPos_Tail = -1;
		}
		else if (!SelectingText())
		{
			// Place tail at the current point:
			m_iBufferPos_Tail = m_iBufferPos;
		}

		m_iBufferPos = (long)pCaption->length();
		m_WantedX = 0.0f;

		UpdateAutoScroll();
		break;
	}
	/**
	 * Conventions for Left/Right when text is selected:
	 *
	 * References:
	 *
	 * Visual Studio
	 *  Visual Studio has the 'newer' approach, used by newer versions of
	 * things, and in newer applications. A left press will always place
	 * the pointer on the left edge of the selection, and then of course
	 * remove the selection. Right will do the exact same thing.
	 * If you have the pointer on the right edge and press right, it will
	 * in other words just remove the selection.
	 *
	 * Windows (eg. Notepad)
	 *  A left press always takes the pointer a step to the left and
	 * removes the selection as if it were never there in the first place.
	 * Right of course does the same thing but to the right.
	 *
	 * I chose the Visual Studio convention. Used also in Word, gtk 2.0, MSN
	 * Messenger.
	 */
	case SDLK_LEFT:
	{
		m_WantedX = 0.f;

		if (shiftKeyPressed || !SelectingText())
		{
			if (!shiftKeyPressed)
				m_iBufferPos_Tail = -1;
			else if (!SelectingText())
				m_iBufferPos_Tail = m_iBufferPos;

			if (m_iBufferPos > 0)
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
	}
	case SDLK_RIGHT:
	{
		m_WantedX = 0.0f;

		if (shiftKeyPressed || !SelectingText())
		{
			if (!shiftKeyPressed)
				m_iBufferPos_Tail = -1;
			else if (!SelectingText())
				m_iBufferPos_Tail = m_iBufferPos;

			if (m_iBufferPos < (int)pCaption->length())
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
	}
	/**
	 * Conventions for Up/Down when text is selected:
	 *
	 * References:
	 *
	 * Visual Studio
	 *  Visual Studio has a very strange approach, down takes you below the
	 * selection to the next row, and up to the one prior to the whole
	 * selection. The weird part is that it is always aligned as the
	 * 'pointer'. I decided this is to much work for something that is
	 * a bit arbitrary
	 *
	 * Windows (eg. Notepad)
	 *  Just like with left/right, the selection is destroyed and it moves
	 * just as if there never were a selection.
	 *
	 * I chose the Notepad convention even though I use the VS convention with
	 * left/right.
	 */
	case SDLK_UP:
	{
		if (!shiftKeyPressed)
			m_iBufferPos_Tail = -1;
		else if (!SelectingText())
			m_iBufferPos_Tail = m_iBufferPos;

		std::list<SRow>::iterator current = m_CharacterPositions.begin();
		while (current != m_CharacterPositions.end())
		{
			if (m_iBufferPos >= current->m_ListStart &&
			    m_iBufferPos <= current->m_ListStart + (int)current->m_ListOfX.size())
				break;

			++current;
		}

		float pos_x;
		if (m_iBufferPos - current->m_ListStart == 0)
			pos_x = 0.f;
		else
			pos_x = current->m_ListOfX[m_iBufferPos - current->m_ListStart - 1];

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
		break;
	}
	case SDLK_DOWN:
	{
		if (!shiftKeyPressed)
			m_iBufferPos_Tail = -1;
		else if (!SelectingText())
			m_iBufferPos_Tail = m_iBufferPos;

		std::list<SRow>::iterator current = m_CharacterPositions.begin();
		while (current != m_CharacterPositions.end())
		{
			if (m_iBufferPos >= current->m_ListStart &&
			    m_iBufferPos <= current->m_ListStart + (int)current->m_ListOfX.size())
				break;

			++current;
		}

		float pos_x;

		if (m_iBufferPos - current->m_ListStart == 0)
			pos_x = 0.f;
		else
			pos_x = current->m_ListOfX[m_iBufferPos - current->m_ListStart - 1];

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
		break;
	}
	case SDLK_PAGEUP:
	{
		GetScrollBar(0).ScrollMinusPlenty();
		UpdateAutoScroll();
		break;
	}
	case SDLK_PAGEDOWN:
	{
		GetScrollBar(0).ScrollPlusPlenty();
		UpdateAutoScroll();
		break;
	}
	default:
	{
		break;
	}
	}
}

InReaction CInput::ManuallyHandleHotkeyEvent(const SDL_Event_* ev)
{
	CStrW* pCaption = (CStrW*)m_Settings["caption"].m_pSetting;
	bool shiftKeyPressed = g_keys[SDLK_RSHIFT] || g_keys[SDLK_LSHIFT];

	std::string hotkey = static_cast<const char*>(ev->ev.user.data1);

	if (hotkey == "paste")
	{
		if (m_Readonly)
			return IN_PASS;

		m_WantedX = 0.0f;

		wchar_t* text = sys_clipboard_get();
		if (text)
		{
			if (SelectingText())
				DeleteCurSelection();

			if (m_iBufferPos == (int)pCaption->length())
				*pCaption += text;
			else
				*pCaption = pCaption->Left(m_iBufferPos) + text +
				pCaption->Right((long) pCaption->length()-m_iBufferPos);

			UpdateText(m_iBufferPos, m_iBufferPos, m_iBufferPos+1);

			m_iBufferPos += (int)wcslen(text);
			UpdateAutoScroll();
			UpdateBufferPositionSetting();

			sys_clipboard_free(text);

			SendEvent(GUIM_TEXTEDIT, "textedit");
		}

		return IN_HANDLED;
	}
	else if (hotkey == "copy" || hotkey == "cut")
	{
		if (m_Readonly && hotkey == "cut")
			return IN_PASS;

		m_WantedX = 0.0f;

		if (SelectingText())
		{
			int virtualFrom;
			int virtualTo;

			if (m_iBufferPos_Tail >= m_iBufferPos)
			{
				virtualFrom = m_iBufferPos;
				virtualTo = m_iBufferPos_Tail;
			}
			else
			{
				virtualFrom = m_iBufferPos_Tail;
				virtualTo = m_iBufferPos;
			}

			CStrW text = (pCaption->Left(virtualTo)).Right(virtualTo - virtualFrom);

			sys_clipboard_set(&text[0]);

			if (hotkey == "cut")
			{
				DeleteCurSelection();
				UpdateAutoScroll();
				SendEvent(GUIM_TEXTEDIT, "textedit");
			}
		}

		return IN_HANDLED;
	}
	else if (hotkey == "text.delete.left")
	{
		if (m_Readonly)
			return IN_PASS;

		m_WantedX = 0.0f;

		if (SelectingText())
			DeleteCurSelection();

		if (!pCaption->empty() && m_iBufferPos != 0)
		{
			m_iBufferPos_Tail = m_iBufferPos;
			CStrW searchString = pCaption->Left(m_iBufferPos);

			// If we are starting in whitespace, adjust position until we get a non whitespace
			while (m_iBufferPos > 0)
			{
				if (!iswspace(searchString[m_iBufferPos - 1]))
					break;

				m_iBufferPos--;
			}

			// If we end up on a punctuation char we just delete it (treat punct like a word)
			if (iswpunct(searchString[m_iBufferPos - 1]))
				m_iBufferPos--;
			else
			{
				// Now we are on a non white space character, adjust position to char after next whitespace char is found
				while (m_iBufferPos > 0)
				{
					if (iswspace(searchString[m_iBufferPos - 1]) || iswpunct(searchString[m_iBufferPos - 1]))
						break;

					m_iBufferPos--;
				}
			}

			UpdateBufferPositionSetting();
			DeleteCurSelection();
			SendEvent(GUIM_TEXTEDIT, "textedit");
		}
		UpdateAutoScroll();
		return IN_HANDLED;
	}
	else if (hotkey == "text.delete.right")
	{
		if (m_Readonly)
			return IN_PASS;

		m_WantedX = 0.0f;

		if (SelectingText())
			DeleteCurSelection();

		if (!pCaption->empty() && m_iBufferPos < (int)pCaption->length())
		{
			// Delete the word to the right of the cursor
			m_iBufferPos_Tail = m_iBufferPos;

			// Delete chars to the right unit we hit whitespace
			while (++m_iBufferPos < (int)pCaption->length())
			{
				if (iswspace((*pCaption)[m_iBufferPos]) || iswpunct((*pCaption)[m_iBufferPos]))
					break;
			}

			// Eliminate any whitespace behind the word we just deleted
			while (m_iBufferPos < (int)pCaption->length())
			{
				if (!iswspace((*pCaption)[m_iBufferPos]))
					break;

				++m_iBufferPos;
			}
			UpdateBufferPositionSetting();
			DeleteCurSelection();
		}
		UpdateAutoScroll();
		SendEvent(GUIM_TEXTEDIT, "textedit");
		return IN_HANDLED;
	}
	else if (hotkey == "text.move.left")
	{
		m_WantedX = 0.0f;

		if (shiftKeyPressed || !SelectingText())
		{
			if (!shiftKeyPressed)
				m_iBufferPos_Tail = -1;
			else if (!SelectingText())
				m_iBufferPos_Tail = m_iBufferPos;

			if (!pCaption->empty() && m_iBufferPos != 0)
			{
				CStrW searchString = pCaption->Left(m_iBufferPos);

				// If we are starting in whitespace, adjust position until we get a non whitespace
				while (m_iBufferPos > 0)
				{
					if (!iswspace(searchString[m_iBufferPos - 1]))
						break;

					m_iBufferPos--;
				}

				// If we end up on a puctuation char we just select it (treat punct like a word)
				if (iswpunct(searchString[m_iBufferPos - 1]))
					m_iBufferPos--;
				else
				{
					// Now we are on a non white space character, adjust position to char after next whitespace char is found
					while (m_iBufferPos > 0)
					{
						if (iswspace(searchString[m_iBufferPos - 1]) || iswpunct(searchString[m_iBufferPos - 1]))
							break;

						m_iBufferPos--;
					}
				}
			}
		}
		else
		{
			if (m_iBufferPos_Tail < m_iBufferPos)
				m_iBufferPos = m_iBufferPos_Tail;

			m_iBufferPos_Tail = -1;
		}

		UpdateBufferPositionSetting();
		UpdateAutoScroll();

		return IN_HANDLED;
	}
	else if (hotkey == "text.move.right")
	{
		m_WantedX = 0.0f;

		if (shiftKeyPressed || !SelectingText())
		{
			if (!shiftKeyPressed)
				m_iBufferPos_Tail = -1;
			else if (!SelectingText())
				m_iBufferPos_Tail = m_iBufferPos;

			if (!pCaption->empty() && m_iBufferPos < (int)pCaption->length())
			{
				// Select chars to the right until we hit whitespace
				while (++m_iBufferPos < (int)pCaption->length())
				{
					if (iswspace((*pCaption)[m_iBufferPos]) || iswpunct((*pCaption)[m_iBufferPos]))
						break;
				}

				// Also select any whitespace following the word we just selected
				while (m_iBufferPos < (int)pCaption->length())
				{
					if (!iswspace((*pCaption)[m_iBufferPos]))
						break;

					++m_iBufferPos;
				}
			}
		}
		else
		{
			if (m_iBufferPos_Tail > m_iBufferPos)
				m_iBufferPos = m_iBufferPos_Tail;

			m_iBufferPos_Tail = -1;
		}

		UpdateBufferPositionSetting();
		UpdateAutoScroll();

		return IN_HANDLED;
	}

	return IN_PASS;
}


void CInput::HandleMessage(SGUIMessage& Message)
{
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

		if (Message.value == CStr("buffer_position"))
		{
			GUI<int>::GetSetting(this, Message.value, m_iBufferPos);
			m_iBufferPos_Tail = -1; // position change resets selection
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

			if (!multiline)
				GetScrollBar(0).SetLength(0.f);
			else
				GetScrollBar(0).SetLength(m_CachedActualSize.bottom - m_CachedActualSize.top);

			UpdateText();
		}

		UpdateAutoScroll();

		if (Message.value == CStr("readonly"))
			GUI<bool>::GetSetting(this, "readonly", m_Readonly);

		break;
	}
	case GUIM_MOUSE_PRESS_LEFT:
	{
		bool scrollbar, multiline;
		GUI<bool>::GetSetting(this, "scrollbar", scrollbar);
		GUI<bool>::GetSetting(this, "multiline", multiline);

		// Check if we're selecting the scrollbar
		if (GetScrollBar(0).GetStyle() && multiline)
		{
			if (GetMousePos().x > m_CachedActualSize.right - GetScrollBar(0).GetStyle()->m_Width)
				break;
		}

		if (m_ComposingText)
			break;

		// Okay, this section is about pressing the mouse and
		//  choosing where the point should be placed. For
		//  instance, if we press between a and b, the point
		//  should of course be placed accordingly. Other
		//  special cases are handled like the input box norms.
		if (g_keys[SDLK_RSHIFT] || g_keys[SDLK_LSHIFT])
			m_iBufferPos = GetMouseHoveringTextPosition();
		else
			m_iBufferPos = m_iBufferPos_Tail = GetMouseHoveringTextPosition();

		m_SelectingText = true;

		UpdateAutoScroll();

		// If we immediately release the button it will just be seen as a click
		//  for the user though.
		break;
	}
	case GUIM_MOUSE_DBLCLICK_LEFT:
	{
		if (m_ComposingText)
			break;

		CStrW* pCaption = (CStrW*)m_Settings["caption"].m_pSetting;

		if (pCaption->empty())
			break;

		m_iBufferPos = m_iBufferPos_Tail = GetMouseHoveringTextPosition();

		if (m_iBufferPos >= (int)pCaption->length())
			m_iBufferPos = m_iBufferPos_Tail = pCaption->length() - 1;

		// See if we are clicking over whitespace
		if (iswspace((*pCaption)[m_iBufferPos]))
		{
			// see if we are in a section of whitespace greater than one character
			if ((m_iBufferPos + 1 < (int) pCaption->length() && iswspace((*pCaption)[m_iBufferPos + 1])) ||
				(m_iBufferPos - 1 > 0 && iswspace((*pCaption)[m_iBufferPos - 1])))
			{
				//
				// We are clicking in an area with more than one whitespace character
				// so we select both the word to the left and then the word to the right
				//
				// [1] First the left
				// skip the whitespace
				while (m_iBufferPos > 0)
				{
					if (!iswspace((*pCaption)[m_iBufferPos - 1]))
						break;

					m_iBufferPos--;
				}
				// now go until we hit white space or punctuation
				while (m_iBufferPos > 0)
				{
					if (iswspace((*pCaption)[m_iBufferPos - 1]))
						break;

					m_iBufferPos--;

					if (iswpunct((*pCaption)[m_iBufferPos]))
						break;
				}

				// [2] Then the right
				// go right until we are not in whitespace
				while (++m_iBufferPos_Tail < (int)pCaption->length())
				{
					if (!iswspace((*pCaption)[m_iBufferPos_Tail]))
						break;
				}

				if (m_iBufferPos_Tail == (int)pCaption->length())
					break;

				// now go to the right until we hit whitespace or punctuation
				while (++m_iBufferPos_Tail < (int)pCaption->length())
				{
					if (iswspace((*pCaption)[m_iBufferPos_Tail]) || iswpunct((*pCaption)[m_iBufferPos_Tail]))
						break;
				}
			}
			else
			{
				// single whitespace so select word to the right
				while (++m_iBufferPos_Tail < (int)pCaption->length())
				{
					if (!iswspace((*pCaption)[m_iBufferPos_Tail]))
						break;
				}

				if (m_iBufferPos_Tail == (int)pCaption->length())
					break;

				// Don't include the leading whitespace
				m_iBufferPos = m_iBufferPos_Tail;

				// now go to the right until we hit whitespace or punctuation
				while (++m_iBufferPos_Tail < (int)pCaption->length())
				{
					if (iswspace((*pCaption)[m_iBufferPos_Tail]) || iswpunct((*pCaption)[m_iBufferPos_Tail]))
						break;
				}
			}
		}
		else
		{
			// clicked on non-whitespace so select current word
			// go until we hit white space or punctuation
			while (m_iBufferPos > 0)
			{
				if (iswspace((*pCaption)[m_iBufferPos - 1]))
					break;

				m_iBufferPos--;

				if (iswpunct((*pCaption)[m_iBufferPos]))
					break;
			}
			// go to the right until we hit whitespace or punctuation
			while (++m_iBufferPos_Tail < (int)pCaption->length())
				if (iswspace((*pCaption)[m_iBufferPos_Tail]) || iswpunct((*pCaption)[m_iBufferPos_Tail]))
					break;
		}
		UpdateAutoScroll();
		break;
	}
	case GUIM_MOUSE_RELEASE_LEFT:
	{
		if (m_SelectingText)
			m_SelectingText = false;
		break;
	}
	case GUIM_MOUSE_MOTION:
	{
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
	}
	case GUIM_LOAD:
	{
		GetScrollBar(0).SetX(m_CachedActualSize.right);
		GetScrollBar(0).SetY(m_CachedActualSize.top);
		GetScrollBar(0).SetZ(GetBufferedZ());
		GetScrollBar(0).SetLength(m_CachedActualSize.bottom - m_CachedActualSize.top);

		CStr scrollbar_style;
		GUI<CStr>::GetSetting(this, "scrollbar_style", scrollbar_style);
		GetScrollBar(0).SetScrollBarStyle(scrollbar_style);

		UpdateText();
		UpdateAutoScroll();

		GUI<bool>::GetSetting(this, "readonly", m_Readonly);
		break;
	}
	case GUIM_GOT_FOCUS:
	{
		m_iBufferPos = 0;
		m_PrevTime = 0.0;
		m_CursorVisState = false;

		// Tell the IME where to draw the candidate list
		SDL_Rect rect;
		rect.h = m_CachedActualSize.GetSize().cy;
		rect.w = m_CachedActualSize.GetSize().cx;
		rect.x = m_CachedActualSize.TopLeft().x;
		rect.y = m_CachedActualSize.TopLeft().y;
		SDL_SetTextInputRect(&rect);
		SDL_StartTextInput();
		break;
	}
	case GUIM_LOST_FOCUS:
	{
		if (m_ComposingText)
		{
			// Simulate a final text editing event to clear the composition
			SDL_Event_ evt;
			evt.ev.type = SDL_TEXTEDITING;
			evt.ev.edit.length = 0;
			evt.ev.edit.start = 0;
			evt.ev.edit.text[0] = 0;
			ManuallyHandleEvent(&evt);
		}
		SDL_StopTextInput();

		m_iBufferPos = -1;
		m_iBufferPos_Tail = -1;
		break;
	}
	default:
	{
		break;
	}
	}
	UpdateBufferPositionSetting();
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

	if (m_CursorBlinkRate > 0.0)
	{
		// check if the cursor visibility state needs to be changed
		double currTime = timer_Time();
		if (currTime - m_PrevTime >= m_CursorBlinkRate)
		{
			m_CursorVisState = !m_CursorVisState;
			m_PrevTime = currTime;
		}
	}
	else
		// should always be visible
		m_CursorVisState = true;

	// First call draw on ScrollBarOwner
	bool scrollbar;
	float buffer_zone;
	bool multiline;
	bool mask;
	GUI<bool>::GetSetting(this, "scrollbar", scrollbar);
	GUI<float>::GetSetting(this, "buffer_zone", buffer_zone);
	GUI<bool>::GetSetting(this, "multiline", multiline);
	GUI<bool>::GetSetting(this, "mask", mask);

	if (scrollbar && multiline)
		IGUIScrollBarOwner::Draw();

	if (!GetGUI())
		return;

	CStrW font_name_w;
	CColor color, color_selected;
	GUI<CStrW>::GetSetting(this, "font", font_name_w);
	GUI<CColor>::GetSetting(this, "textcolor", color);
	GUI<CColor>::GetSetting(this, "textcolor_selected", color_selected);
	CStrIntern font_name(font_name_w.ToUTF8());

	// Get pointer of caption, it might be very large, and we don't
	//  want to copy it continuously.
	CStrW* pCaption = NULL;
	wchar_t mask_char = L'*';
	if (mask)
	{
		CStrW maskStr;
		GUI<CStrW>::GetSetting(this, "mask_char", maskStr);
		if (maskStr.length() > 0)
			mask_char = maskStr[0];
	}
	else
		pCaption = (CStrW*)m_Settings["caption"].m_pSetting;

	CGUISpriteInstance* sprite = NULL;
	CGUISpriteInstance* sprite_selectarea = NULL;
	int cell_id;

	GUI<CGUISpriteInstance>::GetSettingPointer(this, "sprite", sprite);
	GUI<CGUISpriteInstance>::GetSettingPointer(this, "sprite_selectarea", sprite_selectarea);

	GUI<int>::GetSetting(this, "cell_id", cell_id);

	GetGUI()->DrawSprite(*sprite, cell_id, bz, m_CachedActualSize);

	float scroll = 0.f;
	if (scrollbar && multiline)
		scroll = GetScrollBar(0).GetPos();

	CFontMetrics font(font_name);

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
		glEnable(GL_SCISSOR_TEST);
		glScissor(
			cliparea.left * g_GuiScale,
			g_yres - cliparea.bottom * g_GuiScale,
			cliparea.GetWidth() * g_GuiScale,
			cliparea.GetHeight() * g_GuiScale);
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

	CShaderTechniquePtr tech = g_Renderer.GetShaderManager().LoadEffect(str_gui_text);

	CTextRenderer textRenderer(tech->GetShader());
	textRenderer.Font(font_name);

	// Set the Z to somewhat more, so we can draw a selected area between the
	//  the control and the text.
	textRenderer.Translate(
		(float)(int)(m_CachedActualSize.left) + buffer_zone,
		(float)(int)(m_CachedActualSize.top+h) + buffer_zone,
		bz+0.1f);

	// U+FE33: PRESENTATION FORM FOR VERTICAL LOW LINE
	// (sort of like a | which is aligned to the left of most characters)

	float buffered_y = -scroll+buffer_zone;

	// When selecting larger areas, we need to draw a rectangle box
	//  around it, and this is to keep track of where the box
	//  started, because we need to follow the iteration until we
	//  reach the end, before we can actually draw it.
	bool drawing_box = false;
	float box_x = 0.f;

	float x_pointer = 0.f;

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
			if (multiline && buffered_y > m_CachedActualSize.GetHeight())
				break;

			// We might as well use 'i' here to iterate, because we need it
			// (often compared against ints, so don't make it size_t)
			for (int i = 0; i < (int)it->m_ListOfX.size()+2; ++i)
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

				const bool at_end = (i == (int)it->m_ListOfX.size()+1);

				if (drawing_box && (it->m_ListStart + i == VirtualTo || at_end))
				{
					// Depending on if it's just a row change, or if it's
					//  the end of the select box, do slightly different things.
					if (at_end)
					{
						if (it->m_ListStart + i != VirtualFrom)
							// and actually add a white space! yes, this is done in any common input
							x_pointer += (float)font.GetCharacterWidth(L' ');
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
						rect = CRect(
							m_CachedActualSize.left + box_x + buffer_zone,
							m_CachedActualSize.top + buffered_y + (h - ls) / 2,
							m_CachedActualSize.left + x_pointer + buffer_zone,
							m_CachedActualSize.top + buffered_y + (h + ls) / 2);

						if (rect.bottom < m_CachedActualSize.top)
							continue;

						if (rect.top < m_CachedActualSize.top)
							rect.top = m_CachedActualSize.top;

						if (rect.bottom > m_CachedActualSize.bottom)
							rect.bottom = m_CachedActualSize.bottom;
					}
					else // if one-line
					{
						rect = CRect(
							m_CachedActualSize.left + box_x + buffer_zone - m_HorizontalScroll,
							m_CachedActualSize.top + buffered_y + (h - ls) / 2,
							m_CachedActualSize.left + x_pointer + buffer_zone - m_HorizontalScroll,
							m_CachedActualSize.top + buffered_y + (h + ls) / 2);

						if (rect.left < m_CachedActualSize.left)
							rect.left = m_CachedActualSize.left;

						if (rect.right > m_CachedActualSize.right)
							rect.right = m_CachedActualSize.right;
					}

					if (sprite_selectarea)
						GetGUI()->DrawSprite(*sprite_selectarea, cell_id, bz+0.05f, rect);
				}

				if (i < (int)it->m_ListOfX.size())
				{
					if (!mask)
						x_pointer += (float)font.GetCharacterWidth((*pCaption)[it->m_ListStart + i]);
					else
						x_pointer += (float)font.GetCharacterWidth(mask_char);
				}
			}

			if (done)
				break;

			// If we're about to draw a box, and all of a sudden changes
			//  line, we need to draw that line's box, and then reset
			//  the box drawing to the beginning of the new line.
			if (drawing_box)
				box_x = 0.f;
		}
	}

	// Reset some from previous run
	buffered_y = -scroll;

	// Setup initial color (then it might change and change back, when drawing selected area)
	textRenderer.Color(color);

	tech->BeginPass();

	bool using_selected_color = false;

	for (std::list<SRow>::const_iterator it = m_CharacterPositions.begin();
	     it != m_CharacterPositions.end();
	     ++it, buffered_y += ls)
	{
		if (buffered_y + buffer_zone >= -ls || !multiline)
		{
			if (multiline && buffered_y + buffer_zone > m_CachedActualSize.GetHeight())
				break;

			CMatrix3D savedTransform = textRenderer.GetTransform();

			// Text must always be drawn in integer values. So we have to convert scroll
			if (multiline)
				textRenderer.Translate(0.f, -(float)(int)scroll, 0.f);
			else
				textRenderer.Translate(-(float)(int)m_HorizontalScroll, 0.f, 0.f);

			// We might as well use 'i' here, because we need it
			// (often compared against ints, so don't make it size_t)
			for (int i = 0; i < (int)it->m_ListOfX.size()+1; ++i)
			{
				if (!multiline && i < (int)it->m_ListOfX.size())
				{
					if (it->m_ListOfX[i] - m_HorizontalScroll < -buffer_zone)
					{
						// We still need to translate the OpenGL matrix
						if (i == 0)
							textRenderer.Translate(it->m_ListOfX[i], 0.f, 0.f);
						else
							textRenderer.Translate(it->m_ListOfX[i] - it->m_ListOfX[i-1], 0.f, 0.f);

						continue;
					}
				}

				// End of selected area, change back color
				if (SelectingText() && it->m_ListStart + i == VirtualTo)
				{
					using_selected_color = false;
					textRenderer.Color(color);
				}

				// selecting only one, then we need only to draw a cursor.
				if (i != (int)it->m_ListOfX.size() && it->m_ListStart + i == m_iBufferPos && m_CursorVisState)
					textRenderer.Put(0.0f, 0.0f, L"_");

				// Drawing selected area
				if (SelectingText() &&
				    it->m_ListStart + i >= VirtualFrom &&
				    it->m_ListStart + i < VirtualTo &&
				    !using_selected_color)
				{
					using_selected_color = true;
					textRenderer.Color(color_selected);
				}

				if (i != (int)it->m_ListOfX.size())
				{
					if (!mask)
						textRenderer.PrintfAdvance(L"%lc", (*pCaption)[it->m_ListStart + i]);
					else
						textRenderer.PrintfAdvance(L"%lc", mask_char);
				}

				// check it's now outside a one-liner, then we'll break
				if (!multiline && i < (int)it->m_ListOfX.size() &&
				    it->m_ListOfX[i] - m_HorizontalScroll > m_CachedActualSize.GetWidth() - buffer_zone)
					break;
			}

			if (it->m_ListStart + (int)it->m_ListOfX.size() == m_iBufferPos)
			{
				textRenderer.Color(color);
				if (m_CursorVisState)
					textRenderer.PutAdvance(L"_");

				if (using_selected_color)
					textRenderer.Color(color_selected);
			}

			textRenderer.SetTransform(savedTransform);
		}

		textRenderer.Translate(0.f, ls, 0.f);
	}

	textRenderer.Render();

	if (cliparea != CRect())
		glDisable(GL_SCISSOR_TEST);

	tech->EndPass();
}

void CInput::UpdateText(int from, int to_before, int to_after)
{
	CStrW caption;
	CStrW font_name_w;
	float buffer_zone;
	bool multiline;
	bool mask;
	GUI<CStrW>::GetSetting(this, "font", font_name_w);
	GUI<CStrW>::GetSetting(this, "caption", caption);
	GUI<float>::GetSetting(this, "buffer_zone", buffer_zone);
	GUI<bool>::GetSetting(this, "multiline", multiline);
	GUI<bool>::GetSetting(this, "mask", mask);
	CStrIntern font_name(font_name_w.ToUTF8());

	wchar_t mask_char = L'*';
	if (mask)
	{
		CStrW maskStr;
		GUI<CStrW>::GetSetting(this, "mask_char", maskStr);
		if (maskStr.length() > 0)
			mask_char = maskStr[0];
	}

	// Ensure positions are valid after caption changes
	m_iBufferPos = std::min(m_iBufferPos, (int)caption.size());
	m_iBufferPos_Tail = std::min(m_iBufferPos_Tail, (int)caption.size());
	UpdateBufferPositionSetting();

	if (font_name.empty())
	{
		// Destroy everything stored, there's no font, so there can be no data.
		m_CharacterPositions.clear();
		return;
	}

	SRow row;
	row.m_ListStart = 0;

	int to = 0;	// make sure it's initialized

	if (to_before == -1)
		to = (int)caption.length();

	CFontMetrics font(font_name);

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

		std::list<SRow>::iterator destroy_row_from;
		std::list<SRow>::iterator destroy_row_to;
		// Used to check if the above has been set to anything,
		//  previously a comparison like:
		//  destroy_row_from == std::list<SRow>::iterator()
		// ... was used, but it didn't work with GCC.
		bool destroy_row_from_used = false;
		bool destroy_row_to_used = false;

		// Iterate, and remove everything between 'from' and 'to_before'
		//  actually remove the entire lines they are on, it'll all have
		//  to be redone. And when going along, we'll delete a row at a time
		//  when continuing to see how much more after 'to' we need to remake.
		int i = 0;
		for (std::list<SRow>::iterator it = m_CharacterPositions.begin();
		     it != m_CharacterPositions.end();
		     ++it, ++i)
		{
			if (!destroy_row_from_used && it->m_ListStart > from)
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

			if (!destroy_row_to_used && it->m_ListStart > to_before)
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

		if (!destroy_row_from_used)
		{
			destroy_row_from = m_CharacterPositions.end();
			--destroy_row_from;

			// As usual, let's destroy another row back
			if (destroy_row_from != m_CharacterPositions.begin())
				--destroy_row_from;

			current_line = destroy_row_from;
		}

		if (!destroy_row_to_used)
		{
			destroy_row_to = m_CharacterPositions.end();
			check_point_row_start = -1;
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

		std::list<SRow>::iterator temp_it = destroy_row_to;
		--temp_it;

		current_line = m_CharacterPositions.erase(destroy_row_from, destroy_row_to);

		// If there has been a change in number of characters
		//  we need to change all m_ListStart that comes after
		//  the interval we just destroyed. We'll change all
		//  values with the delta change of the string length.
		int delta = to_after - to_before;
		if (delta != 0)
		{
			for (std::list<SRow>::iterator it = current_line;
			     it != m_CharacterPositions.end();
			     ++it)
				it->m_ListStart += delta;

			// Update our check point too!
			check_point_row_start += delta;
			check_point_row_end += delta;

			if (to != (int)caption.length())
				to += delta;
		}
	}

	int last_word_started = from;
	float x_pos = 0.f;

	//if (to_before != -1)
	//	return;

	for (int i = from; i < to; ++i)
	{
		if (caption[i] == L'\n' && multiline)
		{
			if (i == to-1 && to != (int)caption.length())
				break; // it will be added outside

			current_line = m_CharacterPositions.insert(current_line, row);
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

			if (!mask)
				x_pos += (float)font.GetCharacterWidth(caption[i]);
			else
				x_pos += (float)font.GetCharacterWidth(mask_char);

			if (x_pos >= GetTextAreaWidth() && multiline)
			{
				// The following decides whether it will word-wrap a word,
				//  or if it's only one word on the line, where it has to
				//  break the word apart.
				if (last_word_started == row.m_ListStart)
				{
					last_word_started = i;
					row.m_ListOfX.resize(row.m_ListOfX.size() - (i-last_word_started));
					//row.m_ListOfX.push_back(x_pos);
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

				current_line = m_CharacterPositions.insert(current_line, row);
				++current_line;

				// Setup the next row:
				row.m_ListOfX.clear();
				row.m_ListStart = last_word_started;

				i = last_word_started-1;

				x_pos = 0.f;
			}
			else
				// Get width of this character:
				row.m_ListOfX.push_back(x_pos);
		}

		// Check if it's the last iteration, and we're not revising the whole string
		//  because in that case, more word-wrapping might be needed.
		//  also check if the current line isn't the end
		if (to_before != -1 && i == to-1 && current_line != m_CharacterPositions.end())
		{
			// check all rows and see if any existing
			if (row.m_ListStart != check_point_row_start)
			{
				std::list<SRow>::iterator destroy_row_from;
				std::list<SRow>::iterator destroy_row_to;
				// Are used to check if the above has been set to anything,
				//  previously a comparison like:
				//  destroy_row_from == std::list<SRow>::iterator()
				//  was used, but it didn't work with GCC.
				bool destroy_row_from_used = false;
				bool destroy_row_to_used = false;

				// Iterate, and remove everything between 'from' and 'to_before'
				//  actually remove the entire lines they are on, it'll all have
				//  to be redone. And when going along, we'll delete a row at a time
				//  when continuing to see how much more after 'to' we need to remake.
				int i = 0;
				for (std::list<SRow>::iterator it = m_CharacterPositions.begin();
				     it != m_CharacterPositions.end();
				     ++it, ++i)
				{
					if (!destroy_row_from_used && it->m_ListStart > check_point_row_start)
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

					if (!destroy_row_to_used && it->m_ListStart > check_point_row_end)
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
						else
							check_point_row_start = check_point_row_end = -1;

						++destroy_row_to;
						break;
					}
				}

				if (!destroy_row_from_used)
				{
					destroy_row_from = m_CharacterPositions.end();
					--destroy_row_from;

					current_line = destroy_row_from;
				}

				if (!destroy_row_to_used)
				{
					destroy_row_to = m_CharacterPositions.end();
					check_point_row_start = check_point_row_end = -1;
				}

				if (destroy_row_to != m_CharacterPositions.end())
					to = destroy_row_to->m_ListStart; // notice it will iterate [from, to[, so it will never reach to.
				else
					to = (int)caption.length();


				// Set current line, new rows will be added before current_line, so
				//  we'll choose the destroy_row_to, because it won't be deleted
				//  in the coming erase.
				current_line = destroy_row_to;

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
			row.m_ListOfX.resize(row.m_ListOfX.size()-1);
	}

	// add the final row (even if empty)
	m_CharacterPositions.insert(current_line, row);

	bool scrollbar;
	GUI<bool>::GetSetting(this, "scrollbar", scrollbar);
	if (scrollbar)
	{
		GetScrollBar(0).SetScrollRange(m_CharacterPositions.size() * font.GetLineSpacing() + buffer_zone*2.f);
		GetScrollBar(0).SetScrollSpace(m_CachedActualSize.GetHeight());
	}
}

int CInput::GetMouseHoveringTextPosition() const
{
	if (m_CharacterPositions.empty())
		return 0;

	// Return position
	int retPosition;

	float buffer_zone;
	bool multiline;
	GUI<float>::GetSetting(this, "buffer_zone", buffer_zone);
	GUI<bool>::GetSetting(this, "multiline", multiline);

	std::list<SRow>::const_iterator current = m_CharacterPositions.begin();

	CPos mouse = GetMousePos();

	if (multiline)
	{
		CStrW font_name_w;
		bool scrollbar;
		GUI<CStrW>::GetSetting(this, "font", font_name_w);
		GUI<bool>::GetSetting(this, "scrollbar", scrollbar);
		CStrIntern font_name(font_name_w.ToUTF8());

		float scroll = 0.f;
		if (scrollbar)
			scroll = GetScrollBarPos(0);

		// Now get the height of the font.
						// TODO: Get the real font
		CFontMetrics font(font_name);
		float spacing = (float)font.GetLineSpacing();

		// Change mouse position relative to text.
		mouse -= m_CachedActualSize.TopLeft();
		mouse.x -= buffer_zone;
		mouse.y += scroll - buffer_zone;

		int row = (int)((mouse.y) / spacing);

		if (row < 0)
			row = 0;

		if (row > (int)m_CharacterPositions.size()-1)
			row = (int)m_CharacterPositions.size()-1;

		// TODO Gee (2004-11-21): Okay, I need a 'std::list' for some reasons, but I would really like to
		//  be able to get the specific element here. This is hopefully a temporary hack.

		for (int i = 0; i < row; ++i)
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

	retPosition = current->m_ListStart;

	// Okay, now loop through the glyphs to find the appropriate X position
	float dummy;
	retPosition += GetXTextPosition(current, mouse.x, dummy);

	return retPosition;
}

// Does not process horizontal scrolling, 'x' must be modified before inputted.
int CInput::GetXTextPosition(const std::list<SRow>::const_iterator& current, const float& x, float& wanted) const
{
	int ret = 0;
	float previous = 0.f;
	int i = 0;

	for (std::vector<float>::const_iterator it = current->m_ListOfX.begin();
	     it != current->m_ListOfX.end();
	     ++it, ++i)
	{
		if (*it >= x)
		{
			if (x - previous >= *it - x)
				ret += i+1;
			else
				ret += i;

			break;
		}
		previous = *it;
	}
	// If a position wasn't found, we will assume the last
	//  character of that line.
	if (i == (int)current->m_ListOfX.size())
	{
		ret += i;
		wanted = x;
	}
	else
		wanted = 0.f;

	return ret;
}

void CInput::DeleteCurSelection()
{
	CStrW* pCaption = (CStrW*)m_Settings["caption"].m_pSetting;

	int virtualFrom;
	int virtualTo;

	if (m_iBufferPos_Tail >= m_iBufferPos)
	{
		virtualFrom = m_iBufferPos;
		virtualTo = m_iBufferPos_Tail;
	}
	else
	{
		virtualFrom = m_iBufferPos_Tail;
		virtualTo = m_iBufferPos;
	}

	*pCaption = pCaption->Left(virtualFrom) +
				pCaption->Right((long)pCaption->length() - virtualTo);

	UpdateText(virtualFrom, virtualTo, virtualFrom);

	// Remove selection
	m_iBufferPos_Tail = -1;
	m_iBufferPos = virtualFrom;
	UpdateBufferPositionSetting();
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
		CStrW font_name_w;
		bool scrollbar;
		GUI<CStrW>::GetSetting(this, "font", font_name_w);
		GUI<bool>::GetSetting(this, "scrollbar", scrollbar);
		CStrIntern font_name(font_name_w.ToUTF8());

		float scroll = 0.f;
		if (!scrollbar)
			return;

		scroll = GetScrollBar(0).GetPos();

		// Now get the height of the font.
						// TODO: Get the real font
		CFontMetrics font(font_name);
		float spacing = (float)font.GetLineSpacing();
		//float height = font.GetHeight();

		// TODO Gee (2004-11-21): Okay, I need a 'std::list' for some reasons, but I would really like to
		//  be able to get the specific element here. This is hopefully a temporary hack.

		std::list<SRow>::iterator current = m_CharacterPositions.begin();
		int row = 0;
		while (current != m_CharacterPositions.end())
		{
			if (m_iBufferPos >= current->m_ListStart &&
			    m_iBufferPos <= current->m_ListStart + (int)current->m_ListOfX.size())
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
		// If scrolling up
		else if (-scroll + (float)row * spacing < 0.f)
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
				m_iBufferPos > 0)
				x_position = m_CharacterPositions.begin()->m_ListOfX[m_iBufferPos-1];

			// Get complete length:
			x_total = m_CharacterPositions.begin()->m_ListOfX[m_CharacterPositions.begin()->m_ListOfX.size()-1];
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
