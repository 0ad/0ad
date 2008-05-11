/**
 * =========================================================================
 * File        : CConsole.h
 * Project     : 0 A.D.
 * Description : Implements the in-game console with scripting support.
 * =========================================================================
 */

#include <stdarg.h>
#include <string>
#include <deque>
#include <map>

#include "CStr.h"

#include "lib/input.h"
#include "lib/file/vfs/vfs_path.h"

#ifndef INCLUDED_CCONSOLE
#define INCLUDED_CCONSOLE

#define CONSOLE_BUFFER_SIZE 1024 // for text being typed into the console
#define CONSOLE_MESSAGE_SIZE 1024 // for messages being printed into the console

struct JSObject;

typedef void(*fptr)(void);

class CConsole
{
public:
	CConsole();
	~CConsole();

	void SetSize(float X = 300, float Y = 0, float W = 800, float H = 600);
	void UpdateScreenSize(int w, int h);

	void ToggleVisible();
	void SetVisible( bool visible );

	void Update(float DeltaTime);

	void Render();

	void InsertMessage(const wchar_t* szMessage, ...);
	void InsertChar(const int szChar, const wchar_t cooked);

	void SendChatMessage(const wchar_t *szMessage);
	void ReceivedChatMessage(const wchar_t *pSender, const wchar_t *szMessage);

	void SetBuffer(const wchar_t* szMessage, ...);

	void UseHistoryFile( const VfsPath& filename, int historysize );

	// Only returns a pointer to the buffer; copy out of here if you want to keep it.
	const wchar_t* GetBuffer();
	void FlushBuffer();

	void RegisterFunc(fptr F, const wchar_t* szName);

	bool IsActive() { return m_bVisible; }

	int m_iFontHeight;
	int m_iFontWidth;
	int m_iFontOffset; // distance to move up before drawing
	size_t m_charsPerPage;

private:
	float m_fX;
	float m_fY;

	float m_fHeight;
	float m_fWidth;

	// "position" in show/hide animation, how visible the console is (0..1).
	// allows implementing other animations than sliding, e.g. fading in/out.
	float m_fVisibleFrac;
	
	CStrW m_helpText;
	std::map<std::wstring, fptr> m_mapFuncList;

	std::deque<std::wstring> m_deqMsgHistory;
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

	void ToLower(wchar_t* szMessage, size_t iSize = 0);
	void Trim(wchar_t* szMessage, const wchar_t cChar = 32, size_t iSize = 0);

    void DrawHistory(void);
    void DrawWindow(void);
	void DrawBuffer(void);
	void DrawCursor(void);

	bool IsEOB(void) {return (m_iBufferPos == m_iBufferLength);}; //Is end of Buffer?
	bool IsBOB(void) {return (m_iBufferPos == 0);}; //Is beginning of Buffer?
	bool IsFull(void) {return (m_iBufferLength == CONSOLE_BUFFER_SIZE);};
	bool IsEmpty(void) {return (m_iBufferLength == 0);};

	void InsertBuffer(void){InsertMessage(L"%ls", m_szBuffer);};
    void ProcessBuffer(const wchar_t* szLine);

	// Insert message without printf-style formatting, and without
	// length limits
	void InsertMessageRaw(const CStrW& message);

	void LoadHistory();
	void SaveHistory();
	
	JSObject* m_ScriptObject; // to run scripts in, so they can define variables that are retained between commands
};

extern CConsole* g_Console;

extern InReaction conInputHandler(const SDL_Event_* ev);

#endif
