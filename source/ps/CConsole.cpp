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

/*
 * Implements the in-game console with scripting support.
 */

#include "precompiled.h"
#include <wctype.h>

#include "CConsole.h"

#include "graphics/Canvas2D.h"
#include "graphics/FontMetrics.h"
#include "graphics/ShaderManager.h"
#include "graphics/TextRenderer.h"
#include "gui/CGUI.h"
#include "gui/GUIManager.h"
#include "gui/GUIMatrix.h"
#include "lib/ogl.h"
#include "lib/timer.h"
#include "lib/utf8.h"
#include "maths/MathUtil.h"
#include "network/NetClient.h"
#include "network/NetServer.h"
#include "ps/CLogger.h"
#include "ps/ConfigDB.h"
#include "ps/CStrInternStatic.h"
#include "ps/Filesystem.h"
#include "ps/GameSetup/Config.h"
#include "ps/Globals.h"
#include "ps/Hotkey.h"
#include "ps/Profile.h"
#include "ps/Pyrogenesis.h"
#include "renderer/Renderer.h"
#include "scriptinterface/ScriptInterface.h"
#include "scriptinterface/JSON.h"

#include <vector>

namespace
{

// For text being typed into the console.
constexpr int CONSOLE_BUFFER_SIZE = 1024;

const char* CONSOLE_FONT = "mono-10";

} // anonymous namespace

CConsole* g_Console = 0;

CConsole::CConsole()
{
	m_bToggle = false;
	m_bVisible = false;

	m_fVisibleFrac = 0.0f;

	m_szBuffer = new wchar_t[CONSOLE_BUFFER_SIZE];
	FlushBuffer();

	m_iMsgHistPos = 1;
	m_charsPerPage = 0;

	m_prevTime = 0.0;
	m_bCursorVisState = true;
	m_cursorBlinkRate = 0.5;

	InsertMessage("[ 0 A.D. Console v0.14 ]");
	InsertMessage("");
}

CConsole::~CConsole()
{
	delete[] m_szBuffer;
}

void CConsole::Init()
{
	// Initialise console history file
	m_MaxHistoryLines = 200;
	CFG_GET_VAL("console.history.size", m_MaxHistoryLines);

	m_sHistoryFile = L"config/console.txt";
	LoadHistory();

	UpdateScreenSize(g_xres, g_yres);

	// Calculate and store the line spacing
	const CFontMetrics font{CStrIntern(CONSOLE_FONT)};
	m_iFontHeight = font.GetLineSpacing();
	m_iFontWidth = font.GetCharacterWidth(L'C');
	m_charsPerPage = static_cast<size_t>(g_xres / m_iFontWidth);
	// Offset by an arbitrary amount, to make it fit more nicely
	m_iFontOffset = 7;

	m_cursorBlinkRate = 0.5;
	CFG_GET_VAL("gui.cursorblinkrate", m_cursorBlinkRate);
}

void CConsole::UpdateScreenSize(int w, int h)
{
	m_fX = 0;
	m_fY = 0;
	float height = h * 0.6f;
	m_fWidth = w / g_GuiScale;
	m_fHeight = height / g_GuiScale;
}

void CConsole::ToggleVisible()
{
	m_bToggle = true;
	m_bVisible = !m_bVisible;

	// TODO: this should be based on input focus, not visibility
	if (m_bVisible)
		SDL_StartTextInput();
	else
		SDL_StopTextInput();
}

void CConsole::SetVisible(bool visible)
{
	if (visible != m_bVisible)
		m_bToggle = true;
	m_bVisible = visible;
	if (visible)
	{
		m_prevTime = 0.0;
		m_bCursorVisState = false;
	}
}

void CConsole::FlushBuffer()
{
	// Clear the buffer and set the cursor and length to 0
	memset(m_szBuffer, '\0', sizeof(wchar_t) * CONSOLE_BUFFER_SIZE);
	m_iBufferPos = m_iBufferLength = 0;
}

void CConsole::Update(const float deltaRealTime)
{
	if(m_bToggle)
	{
		const float AnimateTime = .30f;
		const float Delta = deltaRealTime / AnimateTime;
		if(m_bVisible)
		{
			m_fVisibleFrac += Delta;
			if(m_fVisibleFrac > 1.0f)
			{
				m_fVisibleFrac = 1.0f;
				m_bToggle = false;
			}
		}
		else
		{
			m_fVisibleFrac -= Delta;
			if(m_fVisibleFrac < 0.0f)
			{
				m_fVisibleFrac = 0.0f;
				m_bToggle = false;
			}
		}
	}
}

void CConsole::Render()
{
	if (! (m_bVisible || m_bToggle) ) return;

	PROFILE3_GPU("console");

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	DrawWindow();

	CShaderTechniquePtr textTech = g_Renderer.GetShaderManager().LoadEffect(str_gui_text);
	textTech->BeginPass();
	CTextRenderer textRenderer(textTech->GetShader());
	textRenderer.Font(CStrIntern(CONSOLE_FONT));
	// animation: slide in from top of screen
	CMatrix3D transform = GetDefaultGuiMatrix();
	const float DeltaY = (1.0f - m_fVisibleFrac) * m_fHeight;
	transform.PostTranslate(m_fX, m_fY - DeltaY, 0.0f); // move to window position
	textRenderer.SetTransform(transform);

	DrawHistory(textRenderer);
	DrawBuffer(textRenderer);

	textRenderer.Render();

	textTech->EndPass();

	glDisable(GL_BLEND);
}

void CConsole::DrawWindow()
{
	std::vector<CVector2D> points = {
		CVector2D{m_fWidth, 0.0f},
		CVector2D{1.0f, 0.0f},
		CVector2D{1.0f, m_fHeight - 1.0f},
		CVector2D{m_fWidth, m_fHeight - 1.0f},
		CVector2D{m_fWidth, 0.0f}
	};
	for (CVector2D& point : points)
		point += CVector2D{m_fX, m_fY - (1.0f - m_fVisibleFrac) * m_fHeight};

	CCanvas2D canvas;
	canvas.DrawRect(CRect(points[1], points[3]), CColor(0.0f, 0.0f, 0.5f, 0.6f));
	canvas.DrawLine(points, 1.0f, CColor(0.5f, 0.5f, 0.0f, 0.6f));

	if (m_fHeight > m_iFontHeight + 4)
	{
		points = {
			CVector2D{0.0f, m_fHeight - static_cast<float>(m_iFontHeight) - 4.0f},
			CVector2D{m_fWidth, m_fHeight - static_cast<float>(m_iFontHeight) - 4.0f}
		};
		canvas.DrawLine(points, 1.0f, CColor(0.5f, 0.5f, 0.0f, 0.6f));
	}
}

void CConsole::DrawHistory(CTextRenderer& textRenderer)
{
	int i = 1;

	std::deque<std::wstring>::iterator Iter; //History iterator

	std::lock_guard<std::mutex> lock(m_Mutex); // needed for safe access to m_deqMsgHistory

	textRenderer.Color(1.0f, 1.0f, 1.0f);

	for (Iter = m_deqMsgHistory.begin();
			Iter != m_deqMsgHistory.end()
				&& (((i - m_iMsgHistPos + 1) * m_iFontHeight) < m_fHeight);
			++Iter)
	{
		if (i >= m_iMsgHistPos)
			textRenderer.Put(9.0f, m_fHeight - (float)m_iFontOffset - (float)m_iFontHeight * (i - m_iMsgHistPos + 1), Iter->c_str());

		i++;
	}
}

// Renders the buffer to the screen.
void CConsole::DrawBuffer(CTextRenderer& textRenderer)
{
	if (m_fHeight < m_iFontHeight)
		return;

	CMatrix3D savedTransform = textRenderer.GetTransform();

	textRenderer.Translate(2.0f, m_fHeight - (float)m_iFontOffset + 1.0f, 0.0f);

	textRenderer.Color(1.0f, 1.0f, 0.0f);
	textRenderer.PutAdvance(L"]");

	textRenderer.Color(1.0f, 1.0f, 1.0f);

	if (m_iBufferPos == 0)
		DrawCursor(textRenderer);

	for (int i = 0; i < m_iBufferLength; i++)
	{
		textRenderer.PrintfAdvance(L"%lc", m_szBuffer[i]);
		if (m_iBufferPos-1 == i)
			DrawCursor(textRenderer);
	}

	textRenderer.SetTransform(savedTransform);
}

void CConsole::DrawCursor(CTextRenderer& textRenderer)
{
	if (m_cursorBlinkRate > 0.0)
	{
		// check if the cursor visibility state needs to be changed
		double currTime = timer_Time();
		if ((currTime - m_prevTime) >= m_cursorBlinkRate)
		{
			m_bCursorVisState = !m_bCursorVisState;
			m_prevTime = currTime;
		}
	}
	else
	{
		// Should always be visible
		m_bCursorVisState = true;
	}

	if(m_bCursorVisState)
	{
		// Slightly translucent yellow
		textRenderer.Color(1.0f, 1.0f, 0.0f, 0.8f);

		// Cursor character is chosen to be an underscore
		textRenderer.Put(0.0f, 0.0f, L"_");

		// Revert to the standard text color
		textRenderer.Color(1.0f, 1.0f, 1.0f);
	}
}

bool CConsole::IsEOB() const
{
	return m_iBufferPos == m_iBufferLength;
}

bool CConsole::IsBOB() const
{
	return m_iBufferPos == 0;
}

bool CConsole::IsFull() const
{
	return m_iBufferLength == CONSOLE_BUFFER_SIZE;
}

bool CConsole::IsEmpty() const
{
	return m_iBufferLength == 0;
}

//Inserts a character into the buffer.
void CConsole::InsertChar(const int szChar, const wchar_t cooked)
{
	static int iHistoryPos = -1;

	if (!m_bVisible) return;

	switch (szChar)
	{
	case SDLK_RETURN:
		iHistoryPos = -1;
		m_iMsgHistPos = 1;
		ProcessBuffer(m_szBuffer);
		FlushBuffer();
		return;

	case SDLK_TAB:
		// Auto Complete
		return;

	case SDLK_BACKSPACE:
		if (IsEmpty() || IsBOB()) return;

		if (m_iBufferPos == m_iBufferLength)
			m_szBuffer[m_iBufferPos - 1] = '\0';
		else
		{
			for (int j = m_iBufferPos-1; j < m_iBufferLength-1; j++)
				m_szBuffer[j] = m_szBuffer[j+1]; // move chars to left
			m_szBuffer[m_iBufferLength-1] = '\0';
		}

		m_iBufferPos--;
		m_iBufferLength--;
		return;

	case SDLK_DELETE:
		if (IsEmpty() || IsEOB()) return;

		if (m_iBufferPos == m_iBufferLength-1)
		{
			m_szBuffer[m_iBufferPos] = '\0';
			m_iBufferLength--;
		}
		else
		{
			if (g_scancodes[SDL_SCANCODE_LCTRL] || g_scancodes[SDL_SCANCODE_RCTRL])
			{
				// Make Ctrl-Delete delete up to end of line
				m_szBuffer[m_iBufferPos] = '\0';
				m_iBufferLength = m_iBufferPos;
			}
			else
			{
				// Delete just one char and move the others left
				for(int j=m_iBufferPos; j<m_iBufferLength-1; j++)
					m_szBuffer[j] = m_szBuffer[j+1];
				m_szBuffer[m_iBufferLength-1] = '\0';
				m_iBufferLength--;
			}
		}

		return;

	case SDLK_HOME:
		if (g_scancodes[SDL_SCANCODE_LCTRL] || g_scancodes[SDL_SCANCODE_RCTRL])
		{
			std::lock_guard<std::mutex> lock(m_Mutex); // needed for safe access to m_deqMsgHistory

			int linesShown = (int)m_fHeight/m_iFontHeight - 4;
			m_iMsgHistPos = Clamp(static_cast<int>(m_deqMsgHistory.size()) - linesShown, 1, static_cast<int>(m_deqMsgHistory.size()));
		}
		else
		{
			m_iBufferPos = 0;
		}
		return;

	case SDLK_END:
		if (g_scancodes[SDL_SCANCODE_LCTRL] || g_scancodes[SDL_SCANCODE_RCTRL])
		{
			m_iMsgHistPos = 1;
		}
		else
		{
			m_iBufferPos = m_iBufferLength;
		}
		return;

	case SDLK_LEFT:
		if (m_iBufferPos) m_iBufferPos--;
		return;

	case SDLK_RIGHT:
		if (m_iBufferPos != m_iBufferLength) m_iBufferPos++;
		return;

	// BEGIN: Buffer History Lookup
	case SDLK_UP:
		if (m_deqBufHistory.size() && iHistoryPos != (int)m_deqBufHistory.size() - 1)
		{
			iHistoryPos++;
			SetBuffer(m_deqBufHistory.at(iHistoryPos).c_str());
			m_iBufferPos = m_iBufferLength;
		}
		return;

	case SDLK_DOWN:
		if (m_deqBufHistory.size())
		{
			if (iHistoryPos > 0)
			{
				iHistoryPos--;
				SetBuffer(m_deqBufHistory.at(iHistoryPos).c_str());
				m_iBufferPos = m_iBufferLength;
			}
			else if (iHistoryPos == 0)
			{
				iHistoryPos--;
				FlushBuffer();
			}
		}
		return;
	// END: Buffer History Lookup

	// BEGIN: Message History Lookup
	case SDLK_PAGEUP:
	{
		std::lock_guard<std::mutex> lock(m_Mutex); // needed for safe access to m_deqMsgHistory

		if (m_iMsgHistPos != (int)m_deqMsgHistory.size()) m_iMsgHistPos++;
		return;
	}

	case SDLK_PAGEDOWN:
		if (m_iMsgHistPos != 1) m_iMsgHistPos--;
		return;
	// END: Message History Lookup

	default: //Insert a character
		if (IsFull()) return;
		if (cooked == 0) return;

		if (IsEOB()) //are we at the end of the buffer?
			m_szBuffer[m_iBufferPos] = cooked; //cat char onto end
		else
		{ //we need to insert
			int i;
			for(i=m_iBufferLength; i>m_iBufferPos; i--)
				m_szBuffer[i] = m_szBuffer[i-1]; // move chars to right
			m_szBuffer[i] = cooked;
		}

		m_iBufferPos++;
		m_iBufferLength++;

		return;
	}
}


void CConsole::InsertMessage(const std::string& message)
{
	// (TODO: this text-wrapping is rubbish since we now use variable-width fonts)

	//Insert newlines to wraparound text where needed
	std::wstring wrapAround = wstring_from_utf8(message.c_str());
	std::wstring newline(L"\n");
	size_t oldNewline=0;
	size_t distance;

	//make sure everything has been initialized
	if ( m_charsPerPage != 0 )
	{
		while ( oldNewline+m_charsPerPage < wrapAround.length() )
		{
			distance = wrapAround.find(newline, oldNewline) - oldNewline;
			if ( distance > m_charsPerPage )
			{
				oldNewline += m_charsPerPage;
				wrapAround.insert( oldNewline++, newline );
			}
			else
				oldNewline += distance+1;
		}
	}
	// Split into lines and add each one individually
	oldNewline = 0;

	{
		std::lock_guard<std::mutex> lock(m_Mutex); // needed for safe access to m_deqMsgHistory

		while ( (distance = wrapAround.find(newline, oldNewline)) != wrapAround.npos)
		{
			distance -= oldNewline;
			m_deqMsgHistory.push_front(wrapAround.substr(oldNewline, distance));
			oldNewline += distance+1;
		}
		m_deqMsgHistory.push_front(wrapAround.substr(oldNewline));
	}
}

const wchar_t* CConsole::GetBuffer()
{
	m_szBuffer[m_iBufferLength] = 0;
	return( m_szBuffer );
}

void CConsole::SetBuffer(const wchar_t* szMessage)
{
	int oldBufferPos = m_iBufferPos;	// remember since FlushBuffer will set it to 0

	FlushBuffer();

	wcsncpy(m_szBuffer, szMessage, CONSOLE_BUFFER_SIZE);
	m_szBuffer[CONSOLE_BUFFER_SIZE-1] = 0;
	m_iBufferLength = static_cast<int>(wcslen(m_szBuffer));
	m_iBufferPos = std::min(oldBufferPos, m_iBufferLength);
}

void CConsole::ProcessBuffer(const wchar_t* szLine)
{
	if (!szLine || wcslen(szLine) <= 0)
		return;

	ENSURE(wcslen(szLine) < CONSOLE_BUFFER_SIZE);

	m_deqBufHistory.push_front(szLine);
	SaveHistory(); // Do this each line for the moment; if a script causes
	               // a crash it's a useful record.

	// Process it as JavaScript
	std::shared_ptr<ScriptInterface> pScriptInterface = g_GUI->GetActiveGUI()->GetScriptInterface();
	ScriptRequest rq(*pScriptInterface);

	JS::RootedValue rval(rq.cx);
	pScriptInterface->Eval(CStrW(szLine).ToUTF8().c_str(), &rval);
	if (!rval.isUndefined())
		InsertMessage(Script::ToString(rq, &rval));
}

void CConsole::LoadHistory()
{
	// note: we don't care if this file doesn't exist or can't be read;
	// just don't load anything in that case.

	// do this before LoadFile to avoid an error message if file not found.
	if (!VfsFileExists(m_sHistoryFile))
		return;

	std::shared_ptr<u8> buf; size_t buflen;
	if (g_VFS->LoadFile(m_sHistoryFile, buf, buflen) < 0)
		return;

	CStr bytes ((char*)buf.get(), buflen);

	CStrW str (bytes.FromUTF8());
	size_t pos = 0;
	while (pos != CStrW::npos)
	{
		pos = str.find('\n');
		if (pos != CStrW::npos)
		{
			if (pos > 0)
				m_deqBufHistory.push_front(str.Left(str[pos-1] == '\r' ? pos - 1 : pos));
			str = str.substr(pos + 1);
		}
		else if (str.length() > 0)
			m_deqBufHistory.push_front(str);
	}
}

void CConsole::SaveHistory()
{
	WriteBuffer buffer;
	const int linesToSkip = (int)m_deqBufHistory.size() - m_MaxHistoryLines;
	std::deque<std::wstring>::reverse_iterator it = m_deqBufHistory.rbegin();
	if(linesToSkip > 0)
		std::advance(it, linesToSkip);
	for (; it != m_deqBufHistory.rend(); ++it)
	{
		CStr8 line = CStrW(*it).ToUTF8();
		buffer.Append(line.data(), line.length());
		static const char newline = '\n';
		buffer.Append(&newline, 1);
	}
	g_VFS->CreateFile(m_sHistoryFile, buffer.Data(), buffer.Size());
}

static bool isUnprintableChar(SDL_Keysym key)
{
	switch (key.sym)
	{
	// We want to allow some, which are handled specially
	case SDLK_RETURN: case SDLK_TAB:
	case SDLK_BACKSPACE: case SDLK_DELETE:
	case SDLK_HOME: case SDLK_END:
	case SDLK_LEFT: case SDLK_RIGHT:
	case SDLK_UP: case SDLK_DOWN:
	case SDLK_PAGEUP: case SDLK_PAGEDOWN:
		return true;

	// Ignore the others
	default:
		return false;
	}
}

InReaction conInputHandler(const SDL_Event_* ev)
{
	if (!g_Console)
		return IN_PASS;

	if (static_cast<int>(ev->ev.type) == SDL_HOTKEYPRESS)
	{
		std::string hotkey = static_cast<const char*>(ev->ev.user.data1);

		if (hotkey == "console.toggle")
		{
			ResetActiveHotkeys();
			g_Console->ToggleVisible();
			return IN_HANDLED;
		}
		else if (g_Console->IsActive() && hotkey == "copy")
		{
			std::string text = utf8_from_wstring(g_Console->GetBuffer());
			SDL_SetClipboardText(text.c_str());
			return IN_HANDLED;
		}
		else if (g_Console->IsActive() && hotkey == "paste")
		{
			char* utf8_text = SDL_GetClipboardText();
			if (!utf8_text)
				return IN_HANDLED;

			std::wstring text = wstring_from_utf8(utf8_text);
			SDL_free(utf8_text);

			for (wchar_t c : text)
				g_Console->InsertChar(0, c);

			return IN_HANDLED;
		}
	}

	if (!g_Console->IsActive())
		return IN_PASS;

	// In SDL2, we no longer get Unicode wchars via SDL_Keysym
	// we use text input events instead and they provide UTF-8 chars
	if (ev->ev.type == SDL_TEXTINPUT)
	{
		// TODO: this could be more efficient with an interface to insert UTF-8 strings directly
		std::wstring wstr = wstring_from_utf8(ev->ev.text.text);
		for (size_t i = 0; i < wstr.length(); ++i)
			g_Console->InsertChar(0, wstr[i]);
		return IN_HANDLED;
	}
	// TODO: text editing events for IME support

	if (ev->ev.type != SDL_KEYDOWN && ev->ev.type != SDL_KEYUP)
		return IN_PASS;

	int sym = ev->ev.key.keysym.sym;

	// Stop unprintable characters (ctrl+, alt+ and escape).
	if (ev->ev.type == SDL_KEYDOWN && isUnprintableChar(ev->ev.key.keysym) &&
		!HotkeyIsPressed("console.toggle"))
	{
		g_Console->InsertChar(sym, 0);
		return IN_HANDLED;
	}

	// We have a probably printable key - we should return HANDLED so it can't trigger hotkeys.
	// However, if Ctrl/Meta modifiers are active (or it's escape), just pass it through instead,
	// assuming that we are indeed trying to trigger hotkeys (e.g. copy/paste).
	// Also ignore the key if we are trying to toggle the console off.
	// See also similar logic in CInput.cpp
	if (EventWillFireHotkey(ev, "console.toggle") ||
		g_scancodes[SDL_SCANCODE_LCTRL] || g_scancodes[SDL_SCANCODE_RCTRL] ||
		g_scancodes[SDL_SCANCODE_LGUI] || g_scancodes[SDL_SCANCODE_RGUI])
		return IN_PASS;

	return IN_HANDLED;
}
