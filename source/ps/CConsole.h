#include <stdarg.h>
#include <string>
#include <deque>
#include <map>

#include "res/font.h"
#include "ogl.h"
#include "lib.h"
#include "sdl.h"


#ifndef CCONSOLE_H
#define CCONSOLE_H

#define BUFFER_SIZE 100
#define FONT_HEIGHT 18

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

	std::map<std::string, fptr> m_mapFuncList;

	std::deque<std::string> m_deqMsgHistory;
	std::deque<std::string> m_deqBufHistory;

	int m_iMsgHistPos;

    char* m_szBuffer;
	int	m_iBufferPos;
	int	m_iBufferLength;

    bool m_bFocus;
	bool m_bVisible;	// console is to be drawn
	bool m_bToggle;		// show/hide animation is currently active

	void ToLower(char* szMessage, uint iSize = 0);
	void Trim(char* szMessage, const char cChar = 32, uint iSize = 0);

    void DrawHistory(void);
    void DrawWindow(void);
	void DrawBuffer(void);
	void DrawCursor(void);

	bool IsEOB(void) {return (m_iBufferPos == m_iBufferLength);}; //Is end of Buffer?
	bool IsBOB(void) {return (m_iBufferPos == 0);}; //Is beginning of Buffer?
	bool IsFull(void) {return (m_iBufferLength == BUFFER_SIZE);};
	bool IsEmpty(void) {return (m_iBufferLength == 0);};

	void InsertBuffer(void){InsertMessage(m_szBuffer);};
    void ProcessBuffer(const char* szLine);

public:
    CConsole(float X = 300, float Y = 0, float W = 800, float H = 600);  //1152x864
	~CConsole();

	void Update(float DeltaTime);

    void Render();

    void InsertMessage(const char* szMessage, ...);
	void InsertChar(const int szChar, const int cooked);

	void SetBuffer(const char* szMessage, ...);
	void FlushBuffer();

	void RegisterFunc(fptr F, const char* szName);

	bool IsActive() { return m_bVisible; }
};

// TODO MT: Better solution to character translation than 'const int cooked'? Anyone?

#endif

