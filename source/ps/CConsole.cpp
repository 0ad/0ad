/* Copyright (C) 2022 Wildfire Games.
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

#include "CConsole.h"

#include "graphics/Canvas2D.h"
#include "graphics/FontMetrics.h"
#include "graphics/TextRenderer.h"
#include "gui/CGUI.h"
#include "gui/GUIManager.h"
#include "lib/code_generation.h"
#include "lib/ogl.h"
#include "lib/timer.h"
#include "lib/utf8.h"
#include "maths/MathUtil.h"
#include "ps/CLogger.h"
#include "ps/ConfigDB.h"
#include "ps/CStrInternStatic.h"
#include "ps/Filesystem.h"
#include "ps/GameSetup/Config.h"
#include "ps/Globals.h"
#include "ps/Hotkey.h"
#include "ps/Profile.h"
#include "ps/Pyrogenesis.h"
#include "ps/VideoMode.h"
#include "scriptinterface/ScriptInterface.h"
#include "scriptinterface/JSON.h"

#include <vector>
#include <wctype.h>

namespace
{

// For text being typed into the console.
constexpr int CONSOLE_BUFFER_SIZE = 1024;

const char* CONSOLE_FONT = "mono-10";

} // anonymous namespace

CConsole* g_Console = 0;

CConsole::CConsole()
{
	m_Toggle = false;
	m_Visible = false;

	m_VisibleFrac = 0.0f;

	m_Buffer = std::make_unique<wchar_t[]>(CONSOLE_BUFFER_SIZE);
	FlushBuffer();

	m_MsgHistPos = 1;
	m_CharsPerPage = 0;

	m_PrevTime = 0.0;
	m_CursorVisState = true;
	m_CursorBlinkRate = 0.5;

	m_QuitHotkeyWasShown = false;

	InsertMessage("[ 0 A.D. Console v0.15 ]");
	InsertMessage("");
}

CConsole::~CConsole() = default;

void CConsole::Init()
{
	// Initialise console history file
	m_MaxHistoryLines = 200;
	CFG_GET_VAL("console.history.size", m_MaxHistoryLines);

	m_HistoryFile = L"config/console.txt";
	LoadHistory();

	UpdateScreenSize(g_xres, g_yres);

	// Calculate and store the line spacing
	const CFontMetrics font{CStrIntern(CONSOLE_FONT)};
	m_FontHeight = font.GetLineSpacing();
	m_FontWidth = font.GetCharacterWidth(L'C');
	m_CharsPerPage = static_cast<size_t>(g_xres / m_FontWidth);
	// Offset by an arbitrary amount, to make it fit more nicely
	m_FontOffset = 7;

	m_CursorBlinkRate = 0.5;
	CFG_GET_VAL("gui.cursorblinkrate", m_CursorBlinkRate);
}

void CConsole::UpdateScreenSize(int w, int h)
{
	m_X = 0;
	m_Y = 0;
	float height = h * 0.6f;
	m_Width = w / g_VideoMode.GetScale();
	m_Height = height / g_VideoMode.GetScale();
}

void CConsole::ShowQuitHotkeys()
{
	if (m_QuitHotkeyWasShown)
		return;

	std::string str;
	for (const std::pair<const SDL_Scancode_, KeyMapping>& key : g_HotkeyMap)
		if (key.second.front().name == "console.toggle")
			str += (str.empty() ? "Press " : " / ") + FindScancodeName(static_cast<SDL_Scancode>(key.first));

	if (!str.empty())
		InsertMessage(str + " to quit.");

	m_QuitHotkeyWasShown = true;
}

void CConsole::ToggleVisible()
{
	m_Toggle = true;
	m_Visible = !m_Visible;

	// TODO: this should be based on input focus, not visibility
	if (m_Visible)
	{
		ShowQuitHotkeys();
		SDL_StartTextInput();
		return;
	}
	SDL_StopTextInput();
}

void CConsole::SetVisible(bool visible)
{
	if (visible != m_Visible)
		m_Toggle = true;
	m_Visible = visible;
	if (visible)
	{
		m_PrevTime = 0.0;
		m_CursorVisState = false;
	}
}

void CConsole::FlushBuffer()
{
	// Clear the buffer and set the cursor and length to 0
	memset(m_Buffer.get(), '\0', sizeof(wchar_t) * CONSOLE_BUFFER_SIZE);
	m_BufferPos = m_BufferLength = 0;
}

void CConsole::Update(const float deltaRealTime)
{
	if (m_Toggle)
	{
		const float AnimateTime = .30f;
		const float Delta = deltaRealTime / AnimateTime;
		if (m_Visible)
		{
			m_VisibleFrac += Delta;
			if (m_VisibleFrac > 1.0f)
			{
				m_VisibleFrac = 1.0f;
				m_Toggle = false;
			}
		}
		else
		{
			m_VisibleFrac -= Delta;
			if (m_VisibleFrac < 0.0f)
			{
				m_VisibleFrac = 0.0f;
				m_Toggle = false;
			}
		}
	}
}

void CConsole::Render(CCanvas2D& canvas)
{
	if (!(m_Visible || m_Toggle))
		return;

	PROFILE3_GPU("console");

	DrawWindow(canvas);

	CTextRenderer textRenderer;
	textRenderer.SetCurrentFont(CStrIntern(CONSOLE_FONT));
	// Animation: slide in from top of screen.
	const float deltaY = (1.0f - m_VisibleFrac) * m_Height;
	textRenderer.Translate(m_X, m_Y - deltaY);

	DrawHistory(textRenderer);
	DrawBuffer(textRenderer);

	canvas.DrawText(textRenderer);
}

void CConsole::DrawWindow(CCanvas2D& canvas)
{
	std::vector<CVector2D> points =
	{
		CVector2D{m_Width, 0.0f},
		CVector2D{1.0f, 0.0f},
		CVector2D{1.0f, m_Height - 1.0f},
		CVector2D{m_Width, m_Height - 1.0f},
		CVector2D{m_Width, 0.0f}
	};
	for (CVector2D& point : points)
		point += CVector2D{m_X, m_Y - (1.0f - m_VisibleFrac) * m_Height};

	canvas.DrawRect(CRect(points[1], points[3]), CColor(0.0f, 0.0f, 0.5f, 0.6f));
	canvas.DrawLine(points, 1.0f, CColor(0.5f, 0.5f, 0.0f, 0.6f));

	if (m_Height > m_FontHeight + 4)
	{
		points = {
			CVector2D{0.0f, m_Height - static_cast<float>(m_FontHeight) - 4.0f},
			CVector2D{m_Width, m_Height - static_cast<float>(m_FontHeight) - 4.0f}
		};
		for (CVector2D& point : points)
			point += CVector2D{m_X, m_Y - (1.0f - m_VisibleFrac) * m_Height};
		canvas.DrawLine(points, 1.0f, CColor(0.5f, 0.5f, 0.0f, 0.6f));
	}
}

void CConsole::DrawHistory(CTextRenderer& textRenderer)
{
	int i = 1;

	std::deque<std::wstring>::iterator it; //History iterator

	std::lock_guard<std::mutex> lock(m_Mutex); // needed for safe access to m_deqMsgHistory

	textRenderer.SetCurrentColor(CColor(1.0f, 1.0f, 1.0f, 1.0f));

	for (it = m_MsgHistory.begin();
			it != m_MsgHistory.end()
				&& (((i - m_MsgHistPos + 1) * m_FontHeight) < m_Height);
			++it)
	{
		if (i >= m_MsgHistPos)
		{
			textRenderer.Put(
				9.0f,
				m_Height - static_cast<float>(m_FontOffset) - static_cast<float>(m_FontHeight) * (i - m_MsgHistPos + 1),
				it->c_str());
		}

		i++;
	}
}

// Renders the buffer to the screen.
void CConsole::DrawBuffer(CTextRenderer& textRenderer)
{
	if (m_Height < m_FontHeight)
		return;

	const CVector2D savedTranslate = textRenderer.GetTranslate();

	textRenderer.Translate(2.0f, m_Height - static_cast<float>(m_FontOffset) + 1.0f);

	textRenderer.SetCurrentColor(CColor(1.0f, 1.0f, 0.0f, 1.0f));
	textRenderer.PutAdvance(L"]");

	textRenderer.SetCurrentColor(CColor(1.0f, 1.0f, 1.0f, 1.0f));

	if (m_BufferPos == 0)
		DrawCursor(textRenderer);

	for (int i = 0; i < m_BufferLength; ++i)
	{
		textRenderer.PrintfAdvance(L"%lc", m_Buffer[i]);
		if (m_BufferPos - 1 == i)
			DrawCursor(textRenderer);
	}

	textRenderer.ResetTranslate(savedTranslate);
}

void CConsole::DrawCursor(CTextRenderer& textRenderer)
{
	if (m_CursorBlinkRate > 0.0)
	{
		// check if the cursor visibility state needs to be changed
		double currTime = timer_Time();
		if ((currTime - m_PrevTime) >= m_CursorBlinkRate)
		{
			m_CursorVisState = !m_CursorVisState;
			m_PrevTime = currTime;
		}
	}
	else
	{
		// Should always be visible
		m_CursorVisState = true;
	}

	if(m_CursorVisState)
	{
		// Slightly translucent yellow
		textRenderer.SetCurrentColor(CColor(1.0f, 1.0f, 0.0f, 0.8f));

		// Cursor character is chosen to be an underscore
		textRenderer.Put(0.0f, 0.0f, L"_");

		// Revert to the standard text color
		textRenderer.SetCurrentColor(CColor(1.0f, 1.0f, 1.0f, 1.0f));
	}
}

bool CConsole::IsEOB() const
{
	return m_BufferPos == m_BufferLength;
}

bool CConsole::IsBOB() const
{
	return m_BufferPos == 0;
}

bool CConsole::IsFull() const
{
	return m_BufferLength == CONSOLE_BUFFER_SIZE;
}

bool CConsole::IsEmpty() const
{
	return m_BufferLength == 0;
}

//Inserts a character into the buffer.
void CConsole::InsertChar(const int szChar, const wchar_t cooked)
{
	static int historyPos = -1;

	if (!m_Visible) return;

	switch (szChar)
	{
	case SDLK_RETURN:
		historyPos = -1;
		m_MsgHistPos = 1;
		ProcessBuffer(m_Buffer.get());
		FlushBuffer();
		return;

	case SDLK_TAB:
		// Auto Complete
		return;

	case SDLK_BACKSPACE:
		if (IsEmpty() || IsBOB()) return;

		if (m_BufferPos == m_BufferLength)
			m_Buffer[m_BufferPos - 1] = '\0';
		else
		{
			for (int j = m_BufferPos-1; j < m_BufferLength - 1; ++j)
				m_Buffer[j] = m_Buffer[j + 1]; // move chars to left
			m_Buffer[m_BufferLength-1] = '\0';
		}

		m_BufferPos--;
		m_BufferLength--;
		return;

	case SDLK_DELETE:
		if (IsEmpty() || IsEOB())
			return;

		if (m_BufferPos == m_BufferLength - 1)
		{
			m_Buffer[m_BufferPos] = '\0';
			m_BufferLength--;
		}
		else
		{
			if (g_scancodes[SDL_SCANCODE_LCTRL] || g_scancodes[SDL_SCANCODE_RCTRL])
			{
				// Make Ctrl-Delete delete up to end of line
				m_Buffer[m_BufferPos] = '\0';
				m_BufferLength = m_BufferPos;
			}
			else
			{
				// Delete just one char and move the others left
				for(int j = m_BufferPos; j < m_BufferLength - 1; ++j)
					m_Buffer[j] = m_Buffer[j + 1];
				m_Buffer[m_BufferLength - 1] = '\0';
				m_BufferLength--;
			}
		}

		return;

	case SDLK_HOME:
		if (g_scancodes[SDL_SCANCODE_LCTRL] || g_scancodes[SDL_SCANCODE_RCTRL])
		{
			std::lock_guard<std::mutex> lock(m_Mutex); // needed for safe access to m_deqMsgHistory

			const int linesShown = static_cast<int>(m_Height / m_FontHeight) - 4;
			m_MsgHistPos = Clamp(static_cast<int>(m_MsgHistory.size()) - linesShown, 1, static_cast<int>(m_MsgHistory.size()));
		}
		else
		{
			m_BufferPos = 0;
		}
		return;

	case SDLK_END:
		if (g_scancodes[SDL_SCANCODE_LCTRL] || g_scancodes[SDL_SCANCODE_RCTRL])
		{
			m_MsgHistPos = 1;
		}
		else
		{
			m_BufferPos = m_BufferLength;
		}
		return;

	case SDLK_LEFT:
		if (m_BufferPos)
			m_BufferPos--;
		return;

	case SDLK_RIGHT:
		if (m_BufferPos != m_BufferLength)
			m_BufferPos++;
		return;

	// BEGIN: Buffer History Lookup
	case SDLK_UP:
		if (m_BufHistory.size() && historyPos != static_cast<int>(m_BufHistory.size()) - 1)
		{
			historyPos++;
			SetBuffer(m_BufHistory.at(historyPos).c_str());
			m_BufferPos = m_BufferLength;
		}
		return;

	case SDLK_DOWN:
		if (m_BufHistory.size())
		{
			if (historyPos > 0)
			{
				historyPos--;
				SetBuffer(m_BufHistory.at(historyPos).c_str());
				m_BufferPos = m_BufferLength;
			}
			else if (historyPos == 0)
			{
				historyPos--;
				FlushBuffer();
			}
		}
		return;
	// END: Buffer History Lookup

	// BEGIN: Message History Lookup
	case SDLK_PAGEUP:
	{
		std::lock_guard<std::mutex> lock(m_Mutex); // needed for safe access to m_deqMsgHistory

		if (m_MsgHistPos != static_cast<int>(m_MsgHistory.size()))
			m_MsgHistPos++;
		return;
	}

	case SDLK_PAGEDOWN:
		if (m_MsgHistPos != 1)
			m_MsgHistPos--;
		return;
	// END: Message History Lookup

	default: //Insert a character
		if (IsFull() || cooked == 0)
			return;

		if (IsEOB()) //are we at the end of the buffer?
			m_Buffer[m_BufferPos] = cooked; //cat char onto end
		else
		{ //we need to insert
			int i;
			for (i = m_BufferLength; i > m_BufferPos; --i)
				m_Buffer[i] = m_Buffer[i - 1]; // move chars to right
			m_Buffer[i] = cooked;
		}

		m_BufferPos++;
		m_BufferLength++;

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
	if (m_CharsPerPage != 0)
	{
		while (oldNewline + m_CharsPerPage < wrapAround.length())
		{
			distance = wrapAround.find(newline, oldNewline) - oldNewline;
			if (distance > m_CharsPerPage)
			{
				oldNewline += m_CharsPerPage;
				wrapAround.insert(oldNewline++, newline);
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
			m_MsgHistory.push_front(wrapAround.substr(oldNewline, distance));
			oldNewline += distance+1;
		}
		m_MsgHistory.push_front(wrapAround.substr(oldNewline));
	}
}

const wchar_t* CConsole::GetBuffer()
{
	m_Buffer[m_BufferLength] = 0;
	return m_Buffer.get();
}

void CConsole::SetBuffer(const wchar_t* szMessage)
{
	int oldBufferPos = m_BufferPos;	// remember since FlushBuffer will set it to 0

	FlushBuffer();

	wcsncpy(m_Buffer.get(), szMessage, CONSOLE_BUFFER_SIZE);
	m_Buffer[CONSOLE_BUFFER_SIZE-1] = 0;
	m_BufferLength = static_cast<int>(wcslen(m_Buffer.get()));
	m_BufferPos = std::min(oldBufferPos, m_BufferLength);
}

void CConsole::ProcessBuffer(const wchar_t* szLine)
{
	if (!szLine || wcslen(szLine) <= 0)
		return;

	ENSURE(wcslen(szLine) < CONSOLE_BUFFER_SIZE);

	m_BufHistory.push_front(szLine);
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
	if (!VfsFileExists(m_HistoryFile))
		return;

	std::shared_ptr<u8> buf; size_t buflen;
	if (g_VFS->LoadFile(m_HistoryFile, buf, buflen) < 0)
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
				m_BufHistory.push_front(str.Left(str[pos-1] == '\r' ? pos - 1 : pos));
			str = str.substr(pos + 1);
		}
		else if (str.length() > 0)
			m_BufHistory.push_front(str);
	}
}

void CConsole::SaveHistory()
{
	WriteBuffer buffer;
	const int linesToSkip = static_cast<int>(m_BufHistory.size()) - m_MaxHistoryLines;
	std::deque<std::wstring>::reverse_iterator it = m_BufHistory.rbegin();
	if(linesToSkip > 0)
		std::advance(it, linesToSkip);
	for (; it != m_BufHistory.rend(); ++it)
	{
		CStr8 line = CStrW(*it).ToUTF8();
		buffer.Append(line.data(), line.length());
		static const char newline = '\n';
		buffer.Append(&newline, 1);
	}

	if (g_VFS->CreateFile(m_HistoryFile, buffer.Data(), buffer.Size()) == INFO::OK)
		ONCE(debug_printf("FILES| Console command history written to '%s'\n", m_HistoryFile.string8().c_str()));
	else
		debug_printf("FILES| Failed to write console command history to '%s'\n", m_HistoryFile.string8().c_str());
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
