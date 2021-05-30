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

#ifndef INCLUDED_CCONSOLE
#define INCLUDED_CCONSOLE

#include "graphics/ShaderProgramPtr.h"
#include "lib/file/vfs/vfs_path.h"
#include "lib/input.h"

#include <deque>
#include <mutex>
#include <stdarg.h>
#include <string>

class CCanvas2D;
class CTextRenderer;

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

	void Init();

	void UpdateScreenSize(int w, int h);

	void ToggleVisible();
	void SetVisible(bool visible);

	/**
	 * @param deltaRealTime Elapsed real time since the last frame.
	 */
	void Update(const float deltaRealTime);

	void Render();

	void InsertChar(const int szChar, const wchar_t cooked);

	void InsertMessage(const std::string& message);

	void SetBuffer(const wchar_t* szMessage);

	// Only returns a pointer to the buffer; copy out of here if you want to keep it.
	const wchar_t* GetBuffer();
	void FlushBuffer();

	bool IsActive() const { return m_bVisible; }

private:
	// Lock for all state modified by InsertMessage
	std::mutex m_Mutex;

	int m_iFontHeight;
	int m_iFontWidth;
	int m_iFontOffset; // distance to move up before drawing
	size_t m_charsPerPage;

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

	bool m_bVisible;	// console is to be drawn
	bool m_bToggle;		// show/hide animation is currently active
	double m_prevTime;	// the previous time the cursor draw state changed (used for blinking cursor)
	bool m_bCursorVisState;	// if the cursor should be drawn or not
	double m_cursorBlinkRate;	// cursor blink rate in seconds, if greater than 0.0

	void DrawWindow(CCanvas2D& canvas);
	void DrawHistory(CTextRenderer& textRenderer);
	void DrawBuffer(CTextRenderer& textRenderer);
	void DrawCursor(CTextRenderer& textRenderer);

	// Is end of Buffer?
	bool IsEOB() const;
	// Is beginning of Buffer?
	bool IsBOB() const;
	bool IsFull() const;
	bool IsEmpty() const;

	void ProcessBuffer(const wchar_t* szLine);

	void LoadHistory();
	void SaveHistory();
};

extern CConsole* g_Console;

extern InReaction conInputHandler(const SDL_Event_* ev);

#endif // INCLUDED_CCONSOLE
