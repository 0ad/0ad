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
 * Implements the in-game console with scripting support.
 */

#ifndef INCLUDED_CCONSOLE
#define INCLUDED_CCONSOLE

#include <stdarg.h>
#include <string>
#include <deque>
#include <map>

#include "lib/input.h"
#include "lib/file/vfs/vfs_path.h"
#include "ps/CStr.h"
#include "ps/ThreadUtil.h"

class CShaderProgram;
typedef shared_ptr<CShaderProgram> CShaderProgramPtr;

class CTextRenderer;

#define CONSOLE_BUFFER_SIZE 1024 // for text being typed into the console
#define CONSOLE_MESSAGE_SIZE 1024 // for messages being printed into the console

#define CONSOLE_FONT "mono-10"

/**
 * In-game console.
 *
 * Thread-safety:
 * - Expected to be constructed/destructed in the main thread.
 * - InsertMessage may be called from any thread while the object is alive.
 */
class CConsole
{
	NONCOPYABLE(CConsole);

public:
	CConsole();
	~CConsole();

	void SetSize(float X = 300, float Y = 0, float W = 800, float H = 600);
	void UpdateScreenSize(int w, int h);

	void ToggleVisible();
	void SetVisible(bool visible);

	void SetCursorBlinkRate(double rate);

	/**
	 * @param deltaRealTime Elapsed real time since the last frame.
	 */
	void Update(const float deltaRealTime);

	void Render();

	void InsertMessage(const wchar_t* szMessage, ...) WPRINTF_ARGS(2);
	void InsertChar(const int szChar, const wchar_t cooked);

	// Insert message without printf-style formatting, and without length limits
	void InsertMessageRaw(const CStrW& message);

	void ReceivedChatMessage(const wchar_t *pSender, const wchar_t *szMessage);

	void SetBuffer(const wchar_t* szMessage);

	void UseHistoryFile(const VfsPath& filename, int historysize);

	// Only returns a pointer to the buffer; copy out of here if you want to keep it.
	const wchar_t* GetBuffer();
	void FlushBuffer();

	bool IsActive() { return m_bVisible; }

	int m_iFontHeight;
	int m_iFontWidth;
	int m_iFontOffset; // distance to move up before drawing
	size_t m_charsPerPage;

private:
	// Lock for all state modified by InsertMessage
	CMutex m_Mutex;

	float m_fX;
	float m_fY;

	float m_fHeight;
	float m_fWidth;

	// "position" in show/hide animation, how visible the console is (0..1).
	// allows implementing other animations than sliding, e.g. fading in/out.
	float m_fVisibleFrac;
	
	std::deque<std::wstring> m_deqMsgHistory; // protected by m_Mutex
	std::deque<std::wstring> m_deqBufHistory;

	int m_iMsgHistPos;

	wchar_t* m_szBuffer;
	int	m_iBufferPos;
	int	m_iBufferLength;

	VfsPath m_sHistoryFile;
	int m_MaxHistoryLines;

	bool m_bFocus;
	bool m_bVisible;	// console is to be drawn
	bool m_bToggle;		// show/hide animation is currently active
	double m_prevTime;	// the previous time the cursor draw state changed (used for blinking cursor)
	bool m_bCursorVisState;	// if the cursor should be drawn or not
	double m_cursorBlinkRate;	// cursor blink rate in seconds, if greater than 0.0

	void ToLower(wchar_t* szMessage, size_t iSize = 0);
	void Trim(wchar_t* szMessage, const wchar_t cChar = 32, size_t iSize = 0);

	void DrawWindow(CShaderProgramPtr& shader);
	void DrawHistory(CTextRenderer& textRenderer);
	void DrawBuffer(CTextRenderer& textRenderer);
	void DrawCursor(CTextRenderer& textRenderer);

	bool IsEOB() { return (m_iBufferPos == m_iBufferLength); } // Is end of Buffer?
	bool IsBOB() { return (m_iBufferPos == 0); } // Is beginning of Buffer?
	bool IsFull() { return (m_iBufferLength == CONSOLE_BUFFER_SIZE); }
	bool IsEmpty() { return (m_iBufferLength == 0); }

	void ProcessBuffer(const wchar_t* szLine);

	void LoadHistory();
	void SaveHistory();
};

extern CConsole* g_Console;

extern InReaction conInputHandler(const SDL_Event_* ev);

#endif
