#include "precompiled.h"
#include <wctype.h>

#include "CConsole.h"

#include "Prometheus.h"
#include "sysdep/sysdep.h"
#include "input.h"

#include "scripting/ScriptingHost.h"

CConsole::CConsole()
{

	m_bToggle = false;
	m_bVisible = false;

	m_fVisibleFrac = 0.0f;

	m_szBuffer = new wchar_t[BUFFER_SIZE];
	FlushBuffer();

	m_iMsgHistPos = 1;

	InsertMessage(L"[ 0 A.D. Console v0.12 ]   type \"\\info\" for help");
	InsertMessage(L"");
}


CConsole::~CConsole()
{
	m_mapFuncList.clear();
	m_deqMsgHistory.clear();
	m_deqBufHistory.clear();
	delete[] m_szBuffer;
}


void CConsole::SetSize(float X, float Y, float W, float H)
{
	m_fX = X;
	m_fY = Y;
	m_fWidth = W;
	m_fHeight = H;
}


void CConsole::ToggleVisible()
{
	m_bToggle = true;
	m_bVisible = !m_bVisible;
}


void CConsole::FlushBuffer(void)
{
	/* Clear the buffer and set the cursor and length to 0 */
	memset(m_szBuffer, '\0', sizeof(wchar_t) * BUFFER_SIZE);
	m_iBufferPos = m_iBufferLength = 0;
}


void CConsole::ToLower(wchar_t* szMessage, uint iSize)
{
	uint L = (uint)wcslen(szMessage);

	if (L <= 0) return;

	if (iSize && iSize < L) L = iSize;

    for(uint i = 0; i < L; i++)
		szMessage[i] = towlower(szMessage[i]);
}


void CConsole::Trim(wchar_t* szMessage, const wchar_t cChar, uint iSize)
{
	size_t L = wcslen(szMessage);
	if(!L)
		return;

	if (iSize && iSize < L) L = iSize;

	wchar_t szChar[2] = { cChar, 0 };

	/* Find the first point at which szChar does not */
	/* exist in the message */
	size_t ofs = wcsspn(szMessage, szChar);
	if(ofs == 0)	// no leading <cChar> chars - we're done
		return;

	// move everything <ofs> chars left, replacing leading cChar chars
	L -= ofs;
	memmove(szMessage, szMessage+ofs, L*sizeof(wchar_t));

	for(ssize_t i = (ssize_t)L; i >= 0; i--)
	{
		szMessage[i] = '\0';
		if (szMessage[i - 1] != cChar) break;
	}
}


void CConsole::RegisterFunc(fptr F, const wchar_t* szName)
{
	// need to allocate a copy - szName may be a const string literal
	// (we'll change it - stripping out spaces and converting to lowercase).
	wchar_t* copy = new wchar_t[BUFFER_SIZE];
	copy[BUFFER_SIZE-1] = '\0';
	wcsncpy(copy, szName, BUFFER_SIZE-1);

	Trim(copy);
	ToLower(copy);

	m_mapFuncList.insert(std::pair<std::wstring, fptr>(copy, F));
	delete[] copy;
}


void CConsole::Update(const float DeltaTime)
{
	if(m_bToggle)
	{
		const float AnimateTime = .30f;
		const float Delta = DeltaTime / AnimateTime;
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

//Render Manager.
void CConsole::Render()
{
	if (! (m_bVisible || m_bToggle) ) return;

	// animation: slide in from top of screen
	const float MaxY = m_fHeight;
	const float DeltaY = (1.0f - m_fVisibleFrac) * MaxY;

	glTranslatef(m_fX, m_fY + DeltaY, 0.0f); //Move to window position

    DrawWindow();
    DrawHistory();
	DrawBuffer();
}


void CConsole::DrawWindow(void)
{
	/* TODO:  Add texturing */
	glDisable(GL_TEXTURE_2D);

	glPushMatrix();

		/* Draw Background */
		/* Set the color to a translucent blue */
		glColor4f(0.0f, 0.0f, 0.5f, 0.6f);
		glBegin(GL_QUADS);
			glVertex2f(0.0f,		0.0f);
			glVertex2f(m_fWidth,	0.0f);
			glVertex2f(m_fWidth,	m_fHeight);
			glVertex2f(0.0f,		m_fHeight);
		glEnd();

		/* Draw Border */
		/* Set the color to a translucent yellow */
		glColor4f(0.5f, 0.5f, 0.0f, 0.6f);
		glBegin(GL_LINES);
			glVertex2f(0.0f,		0.0f);
			glVertex2f(m_fWidth,	0.0f);

			glVertex2f(m_fWidth,	0.0f);
			glVertex2f(m_fWidth,	m_fHeight);

			glVertex2f(m_fWidth,	m_fHeight);
			glVertex2f(0.0f,		m_fHeight);

			glVertex2f(0.0f,		m_fHeight);
			glVertex2f(0.0f,		0.0f);

			if (m_fHeight > m_iFontHeight + 4)
			{
				glVertex2f(0.0f,		(GLfloat)(m_iFontHeight + 4));
				glVertex2f(m_fWidth,	(GLfloat)(m_iFontHeight + 4));
			}
		glEnd();
	glPopMatrix();

	glEnable(GL_TEXTURE_2D);
}


void CConsole::DrawHistory(void) {
	int i = 1;
	
	std::deque<std::wstring>::iterator Iter; //History iterator

	glPushMatrix();
		glColor3f(1.0f, 1.0f, 1.0f); //Set color of text
		glTranslatef(9.0f, (float)m_iFontOffset, 0.0f); //move away from the border

		// Draw the text upside-down, because it's aligned with
		// the GUI (which uses the top-left as (0,0))
		glScalef(1.0f, -1.0f, 1.0f);

		for (Iter = m_deqMsgHistory.begin();
			 Iter != m_deqMsgHistory.end()
				 && (((i - m_iMsgHistPos + 1) * m_iFontHeight) < (m_fHeight - m_iFontHeight));
			 Iter++)
		{
			if (i >= m_iMsgHistPos){
				glTranslatef(0.0f, -(float)m_iFontHeight, 0.0f);

				glPushMatrix();
					glwprintf(L"%ls", Iter->data());
				glPopMatrix();
			}

			i++;
		}
	glPopMatrix();
}

//Renders the buffer to the screen.
void CConsole::DrawBuffer(void)
{
	if (m_fHeight < m_iFontHeight) return;

	glPushMatrix();
		glColor3f(1.0f, 1.0f, 0.0f);
		glTranslatef(2.0f, (float)m_iFontOffset, 0);
		glScalef(1.0f, -1.0f, 1.0f);

		glwprintf(L"]");

		glColor3f(1.0f, 1.0f, 1.0f);
		if (m_iBufferPos==0) DrawCursor();

		for (int i = 0; i < m_iBufferLength; i++){
				glwprintf(L"%lc", m_szBuffer[i]);
				if (m_iBufferPos-1==i) DrawCursor();
		}
	glPopMatrix();
}


void CConsole::DrawCursor(void)
{
	glPushMatrix();
		glwprintf(L"_");
	glPopMatrix();
}


//Inserts a character into the buffer.
void CConsole::InsertChar(const int szChar, const wchar_t cooked )
{
	static int iHistoryPos = -1;

	if (!m_bVisible) return;

	switch (szChar){
		case '\r':
		case '\n':
			iHistoryPos = -1;
			m_iMsgHistPos = 1;
            ProcessBuffer(m_szBuffer);
			FlushBuffer();
			return;

		case '\t':
			/* Auto Complete */
			return;

		case '\b':
			if (IsEmpty() || IsBOB()) return;

			if (m_iBufferPos == m_iBufferLength)
				m_szBuffer[m_iBufferPos - 1] = '\0';
			else{
                for(int j=m_iBufferPos-1; j<m_iBufferLength-1; j++)
					m_szBuffer[j] = m_szBuffer[j+1]; // move chars to left
				m_szBuffer[m_iBufferLength-1] = '\0';
            }

			m_iBufferPos--;
			m_iBufferLength--;
			return;

		case SDLK_DELETE:
			if (IsEmpty() || IsEOB()) return;

			if (m_iBufferPos == m_iBufferLength-1)
				m_szBuffer[m_iBufferPos] = '\0';
			else{
				for(int j=m_iBufferPos; j<m_iBufferLength-1; j++)
					m_szBuffer[j] = m_szBuffer[j+1]; // move chars to left
				m_szBuffer[m_iBufferLength-1] = '\0';
			}

			m_iBufferLength--;
			return;

		case SDLK_HOME:
			m_iBufferPos = 0;
			return;

		case SDLK_END:
			m_iBufferPos = m_iBufferLength;
			return;

		case SDLK_LEFT:
			if (m_iBufferPos) m_iBufferPos--;
			return;

		case SDLK_RIGHT:
			if (m_iBufferPos != m_iBufferLength) m_iBufferPos++;
			return;

		/* BEGIN: Buffer History Lookup */
		case SDLK_UP:
			if (m_deqBufHistory.size() && iHistoryPos != (int)m_deqBufHistory.size() - 1)
			{
				iHistoryPos++;
				SetBuffer(m_deqBufHistory.at(iHistoryPos).data());
			}
			return;

		case SDLK_DOWN:
			if (iHistoryPos != -1) iHistoryPos--;

			if (iHistoryPos != -1)
				SetBuffer(m_deqBufHistory.at(iHistoryPos).data());
			else FlushBuffer();
			return;
		/* END: Buffer History Lookup */

		/* BEGIN: Message History Lookup */
		case SDLK_PAGEUP:
			if (m_iMsgHistPos != (int)m_deqMsgHistory.size()) m_iMsgHistPos++;
			return;

		case SDLK_PAGEDOWN:
			if (m_iMsgHistPos != 1) m_iMsgHistPos--;
			return;
		/* END: Message History Lookup */

		default: //Insert a character
			if (IsFull()) return;
			if (cooked == 0) return;

			if (IsEOB()) //are we at the end of the buffer?
				m_szBuffer[m_iBufferPos] = cooked; //cat char onto end
			else{ //we need to insert
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


void CConsole::InsertMessage(const wchar_t* szMessage, ...)
{
	va_list args;
	wchar_t* szBuffer = new wchar_t[MESSAGE_SIZE];

	va_start(args, szMessage);
	if (vswprintf(szBuffer, MESSAGE_SIZE, szMessage, args) == -1)
	{
		debug_out("Error printfing console message (buffer size exceeded?)\n");

		// Make it obvious that the text was trimmed (assuming it was)
		wcscpy(szBuffer+MESSAGE_SIZE-4, L"...");
	}
	va_end(args);

	// Split into lines and add each one individually
	wchar_t* lineStart = szBuffer;
	wchar_t* lineEnd;
	while ( (lineEnd = wcschr(lineStart, '\n')) != NULL)
	{
		m_deqMsgHistory.push_front(std::wstring(lineStart, lineEnd));
		lineStart = lineEnd+1;
	}
	m_deqMsgHistory.push_front(std::wstring(lineStart));

	delete[] szBuffer;
}


void CConsole::SetBuffer(const wchar_t* szMessage, ...)
{
	va_list args;
	wchar_t* szBuffer = new wchar_t[BUFFER_SIZE];

	va_start(args, szMessage);
		vswprintf(szBuffer, BUFFER_SIZE, szMessage, args);
	va_end(args);

	FlushBuffer();

	wcsncpy(m_szBuffer, szMessage, BUFFER_SIZE);
	m_iBufferLength = m_iBufferPos = (int)wcslen(m_szBuffer);

	delete[] szBuffer;
}


void CConsole::ProcessBuffer(const wchar_t* szLine){
	if (szLine == NULL) return;
	if (wcslen(szLine) <= 0) return;
	
	assert(wcslen(szLine) < BUFFER_SIZE);

	m_deqBufHistory.push_front(szLine);

	wchar_t* szCommand = new wchar_t[BUFFER_SIZE];
    memset(szCommand, '\0', sizeof(wchar_t) * BUFFER_SIZE);

	std::map<std::wstring, fptr>::iterator Iter;

	if (szLine[0] == '\\'){
		swscanf(szLine, L"\\%ls", szCommand);
		Trim(szCommand);
		ToLower(szCommand);

		if (!wcscmp(szCommand, L"info")){
			InsertMessage(L"");
			InsertMessage(L"[Information]");
			InsertMessage(L"   -View commands \"\\commands\"");
			InsertMessage(L"   -Call command \"\\<command>\"");
			InsertMessage(L"   -Say \"<string>\"");
			InsertMessage(L"");
		}else if (!wcscmp(szCommand, L"commands")){
			InsertMessage(L"");
			InsertMessage(L"[Commands]");

			if (!m_mapFuncList.size()) InsertMessage(L"   (none registered)");

			for (Iter = m_mapFuncList.begin(); Iter != m_mapFuncList.end(); Iter++)
				InsertMessage(L"   \\%ls", Iter->first.data());

			InsertMessage(L"");
		}else{
			Iter = m_mapFuncList.find(szCommand);
			if (Iter == m_mapFuncList.end())
				InsertMessage(L"unknown command <%ls>", szCommand);
			else
				Iter->second();
		}
	}
	else if( szLine[0] == ':' )
	{
		// Process it as JavaScript

		// Convert Unicode to 8-bit sort-of-ASCII
		size_t len = wcstombs(NULL, szLine, 0);
		assert(len != (size_t)-1 && "Cannot convert unicode->multibyte");
		char* szMBLine = new char[len+1];
		wcstombs(szMBLine, szLine, len+1);

		g_ScriptingHost.ExecuteScript( szMBLine + 1 );

		delete[] szMBLine;
	}
	else if( szLine[0] == '?' )
	{
		// Process it as JavaScript and display the result

		// Convert Unicode to 8-bit sort-of-ASCII
		size_t len = wcstombs(NULL, szLine, 0);
		assert(len != (size_t)-1 && "Cannot convert unicode->multibyte");
		char* szMBLine = new char[len+1];
		wcstombs(szMBLine, szLine, len+1);

		jsval rval = g_ScriptingHost.ExecuteScript( szMBLine + 1 );
		if( rval )
			InsertMessage( L"%hs", g_ScriptingHost.ValueToString( rval ).c_str() );

		delete[] szMBLine;
	}
	else InsertMessage(L"<say>: %ls", szLine);

	delete[] szCommand;
}


#include "sdl.h"

extern CConsole* g_Console;

extern void Die(int err, const wchar_t* fmt, ...);

int conInputHandler(const SDL_Event* ev)
{
	if(ev->type != SDL_KEYDOWN)
		return EV_PASS;

	SDLKey sym = ev->key.keysym.sym;

	// toggle
	if(sym == SDLK_F1)
	{
		g_Console->ToggleVisible();
		return EV_HANDLED;
			// bail, because we'd fall under if(sym) .. else below
	}

	if(!g_Console->IsActive())
		return EV_PASS;

	// paste from clipboard
	if(sym == SDLK_INSERT)
	{
		wchar_t* text = clipboard_get();
		if(text)
		{
			for(wchar_t* c = text; *c; c++)
				g_Console->InsertChar(0, *c);

			clipboard_free(text);
		}
	}
	else
		g_Console->InsertChar(sym, (wchar_t)ev->key.keysym.unicode );

	return EV_HANDLED;
}
