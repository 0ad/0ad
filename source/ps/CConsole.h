/***************************************************************************************
	AUTHOR:			John M. Mena
	EMAIL:			JohnMMena@hotmail.com
	FILE:			CConsole.h
	CREATED:		12/3/03
	COMPLETED:		NULL

	DESCRIPTION:	The class CConsole provides an interface to the
					scripting abilities of an engine.
****************************************************************************************/


#include <stdarg.h>
#include <string>
#include <deque>
#include <map>

#include "CStr.h"

#include "lib/input.h"

#ifndef CCONSOLE_H
#define CCONSOLE_H

#define CONSOLE_BUFFER_SIZE 1024 // for text being typed into the console
#define CONSOLE_MESSAGE_SIZE 1024 // for messages being printed into the console

typedef void(*fptr)(void);

class CConsole
{
private:
	float m_fX;
	float m_fY;

	float m_fHeight;
	float m_fWidth;

	// "position" in show/hide animation, how visible the console is (0..1).
	// allows implementing other animations than sliding, e.g. fading in/out.
	float m_fVisibleFrac;

	std::map<std::wstring, fptr> m_mapFuncList;

	std::deque<std::wstring> m_deqMsgHistory;
	std::deque<std::wstring> m_deqBufHistory;

	int m_iMsgHistPos;

    wchar_t* m_szBuffer;
	int	m_iBufferPos;
	int	m_iBufferLength;

	CStr m_sHistoryFile;
	int m_MaxHistoryLines;

    bool m_bFocus;
	bool m_bVisible;	// console is to be drawn
	bool m_bToggle;		// show/hide animation is currently active

	void ToLower(wchar_t* szMessage, uint iSize = 0);
	void Trim(wchar_t* szMessage, const wchar_t cChar = 32, uint iSize = 0);

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

	void LoadHistory();
	void SaveHistory();

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

	void UseHistoryFile( CStr filename, int historysize );

	// Only returns a pointer to the buffer; copy out of here if you want to keep it.
	const wchar_t* GetBuffer();
	void FlushBuffer();

	void RegisterFunc(fptr F, const wchar_t* szName);

	bool IsActive() { return m_bVisible; }

	int m_iFontHeight;
	int m_iFontOffset; // distance to move up before drawing
};

extern CConsole* g_Console;

extern InReaction conInputHandler(const SDL_Event* ev);

#endif
