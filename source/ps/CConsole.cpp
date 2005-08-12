#include "precompiled.h"
#include <wctype.h>

#include "CConsole.h"
#include "CLogger.h"

#include "Pyrogenesis.h"
#include "sysdep/sysdep.h"
#include "input.h"
#include "Hotkey.h"
#include "scripting/ScriptingHost.h"

#include "Network/Client.h"
#include "Network/Server.h"

#include "lib/res/file/vfs.h"

#include "Interact.h"

extern bool keys[SDLK_LAST];

CConsole* g_Console = 0;

CConsole::CConsole()
{

	m_bToggle = false;
	m_bVisible = false;

	m_fVisibleFrac = 0.0f;

	m_szBuffer = new wchar_t[CONSOLE_BUFFER_SIZE];
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

void CConsole::UpdateScreenSize(int w, int h)
{
	float height = h * 0.6f;
	SetSize(0, h-height, (float)w, height);
}


void CConsole::ToggleVisible()
{
	m_bToggle = true;
	m_bVisible = !m_bVisible;
}

void CConsole::SetVisible( bool visible )
{
	if( visible != m_bVisible )
		m_bToggle = true;
	m_bVisible = visible;
}

void CConsole::FlushBuffer(void)
{
	// Clear the buffer and set the cursor and length to 0
	memset(m_szBuffer, '\0', sizeof(wchar_t) * CONSOLE_BUFFER_SIZE);
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

	// Find the first point at which szChar does not
	// exist in the message
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
	wchar_t copy[CONSOLE_BUFFER_SIZE];
	copy[CONSOLE_BUFFER_SIZE-1] = '\0';
	wcsncpy(copy, szName, CONSOLE_BUFFER_SIZE-1);

	Trim(copy);
	ToLower(copy);

	m_mapFuncList.insert(std::pair<std::wstring, fptr>(copy, F));
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
	// TODO:  Add texturing
	glDisable(GL_TEXTURE_2D);

	glPushMatrix();

		// Draw Background
		// Set the color to a translucent blue
		glColor4f(0.0f, 0.0f, 0.5f, 0.6f);
		glBegin(GL_QUADS);
			glVertex2f(0.0f,		0.0f);
			glVertex2f(m_fWidth-1.0f,	0.0f);
			glVertex2f(m_fWidth-1.0f,	m_fHeight-1.0f);
			glVertex2f(0.0f,		m_fHeight-1.0f);
		glEnd();

		// Draw Border
		// Set the color to a translucent yellow
		glColor4f(0.5f, 0.5f, 0.0f, 0.6f);
		glBegin(GL_LINE_LOOP);
			glVertex2f(0.0f,		0.0f);
			glVertex2f(m_fWidth-1.0f,	0.0f);
			glVertex2f(m_fWidth-1.0f,	m_fHeight-1.0f);
			glVertex2f(0.0f,		m_fHeight-1.0f);
		glEnd();

		if (m_fHeight > m_iFontHeight + 4)
		{
			glBegin(GL_LINES);
				glVertex2f(0.0f,		(GLfloat)(m_iFontHeight + 4));
				glVertex2f(m_fWidth,	(GLfloat)(m_iFontHeight + 4));
			glEnd();
		}
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
				 && (((i - m_iMsgHistPos + 1) * m_iFontHeight) < m_fHeight);
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
		// Slightly translucent yellow
		glColor4f(1.0f, 1.0f, 0.0f, 0.8f);

		// U+FE33: PRESENTATION FORM FOR VERTICAL LOW LINE
		// (sort of like a | which is aligned to the left of most characters)
		glwprintf(L"%lc", 0xFE33);

		// Revert to the standard text colour
		glColor3f(1.0f, 1.0f, 1.0f);
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
			// Auto Complete
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
			if (keys[SDLK_RCTRL] || keys[SDLK_LCTRL])
			{
				int linesShown = (int)m_fHeight/m_iFontHeight - 4;
				m_iMsgHistPos = clamp((int)m_deqMsgHistory.size() - linesShown, 1, (int)m_deqMsgHistory.size());
			}
			else
			{
				m_iBufferPos = 0;
			}
			return;

		case SDLK_END:
			if (keys[SDLK_RCTRL] || keys[SDLK_LCTRL])
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
				SetBuffer(m_deqBufHistory.at(iHistoryPos).data());
			}
			return;

		case SDLK_DOWN:
			if (iHistoryPos != -1) iHistoryPos--;

			if (iHistoryPos != -1)
				SetBuffer(m_deqBufHistory.at(iHistoryPos).data());
			else FlushBuffer();
			return;
		// END: Buffer History Lookup

		// BEGIN: Message History Lookup
		case SDLK_PAGEUP:
			if (m_iMsgHistPos != (int)m_deqMsgHistory.size()) m_iMsgHistPos++;
			return;

		case SDLK_PAGEDOWN:
			if (m_iMsgHistPos != 1) m_iMsgHistPos--;
			return;
		// END: Message History Lookup

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
	wchar_t szBuffer[CONSOLE_MESSAGE_SIZE];

	va_start(args, szMessage);
	if (vswprintf(szBuffer, CONSOLE_MESSAGE_SIZE, szMessage, args) == -1)
	{
		debug_printf("Error printfing console message (buffer size exceeded?)\n");

		// Make it obvious that the text was trimmed (assuming it was)
		wcscpy(szBuffer+CONSOLE_MESSAGE_SIZE-4, L"...");
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
}

const wchar_t* CConsole::GetBuffer()
{
	m_szBuffer[m_iBufferLength] = 0;
	return( m_szBuffer );
}

void CConsole::SetBuffer(const wchar_t* szMessage, ...)
{
	va_list args;
	wchar_t szBuffer[CONSOLE_BUFFER_SIZE];

	va_start(args, szMessage);
		vswprintf(szBuffer, CONSOLE_BUFFER_SIZE, szMessage, args);
	va_end(args);

	FlushBuffer();

	wcsncpy(m_szBuffer, szMessage, CONSOLE_BUFFER_SIZE);
	m_iBufferLength = m_iBufferPos = (int)wcslen(m_szBuffer);
}

void CConsole::UseHistoryFile( CStr filename, int max_history_lines )
{
	m_MaxHistoryLines = max_history_lines;

	m_sHistoryFile = filename;
	LoadHistory();
}

void CConsole::ProcessBuffer(const wchar_t* szLine){
	if (szLine == NULL) return;
	if (wcslen(szLine) <= 0) return;
	
	debug_assert(wcslen(szLine) < CONSOLE_BUFFER_SIZE);

	m_deqBufHistory.push_front(szLine);
	SaveHistory(); // Do this each line for the moment; if a script causes
	               // a crash it's a useful record.

	wchar_t szCommand[CONSOLE_BUFFER_SIZE];
	memset(szCommand, '\0', sizeof(wchar_t) * CONSOLE_BUFFER_SIZE);

	std::map<std::wstring, fptr>::iterator Iter;

	if (szLine[0] == '\\')
	{
		swscanf(szLine, L"\\%ls", szCommand);
		Trim(szCommand);
		ToLower(szCommand);

		if (!wcscmp(szCommand, L"info"))
		{
			InsertMessage(L"");
			InsertMessage(L"[Information]");
			InsertMessage(L"   -View commands \"\\commands\"");
			InsertMessage(L"   -Call command \"\\<command>\"");
			InsertMessage(L"   -Say \"<string>\"");
			InsertMessage(L"");
		}
		else if (!wcscmp(szCommand, L"commands"))
		{
			InsertMessage(L"");
			InsertMessage(L"[Commands]");

			if (!m_mapFuncList.size()) InsertMessage(L"   (none registered)");

			for (Iter = m_mapFuncList.begin(); Iter != m_mapFuncList.end(); Iter++)
				InsertMessage(L"   \\%ls", Iter->first.data());

			InsertMessage(L"");
		}
		else
		{
			Iter = m_mapFuncList.find(szCommand);
			if (Iter == m_mapFuncList.end())
				InsertMessage(L"unknown command <%ls>", szCommand);
			else
				Iter->second();
		}
	}
	else if (szLine[0] == ':')
	{
		// Process it as JavaScript

		// (Cheating) run it as the first selected entity, if there is one.
		JSObject* RunAs = NULL;
		if( g_Selection.m_selected.size() )
			RunAs = g_Selection.m_selected[0]->GetScript();

		g_ScriptingHost.ExecuteScript( CStrW( szLine + 1 ), L"Console", RunAs );

	}
	else if (szLine[0] == '?')
	{
		// Process it as JavaScript and display the result

		// (Cheating) run it as the first selected entity, if there is one.
		JSObject* RunAs = NULL;
		if( g_Selection.m_selected.size() )
			RunAs = g_Selection.m_selected[0]->GetScript();

		jsval rval = g_ScriptingHost.ExecuteScript( CStrW( szLine + 1 ), L"Console", RunAs );
		if (rval)
		{
			try {
				InsertMessage( L"%ls", g_ScriptingHost.ValueToUCString( rval ).c_str() );
			} catch (PSERROR_Scripting_ConversionFailed) {
				InsertMessage( L"%hs", "<error converting return value to string>" );
			}
		}
	}
	else
		SendChatMessage(szLine);
}

void CConsole::LoadHistory()
{
	void* buffer; unsigned int buflen;
	Handle h = vfs_load( m_sHistoryFile, buffer, buflen );
	if( h > 0 )
	{
		CStr bytes( (char*)buffer, buflen );
		CStrW str( bytes.FromUTF8() );
		size_t pos = 0;
		while( pos != -1 )
		{
			pos = str.find( '\n' );
			if( pos != -1 )
			{
				if( pos > 0 )
					m_deqBufHistory.push_front( str.Left( str[pos-1] == '\r' ? pos - 1 : pos ) );
				str = str.GetSubstring( pos + 1, str.npos );
			}
			else if( str.Length() > 0 )
				m_deqBufHistory.push_front( str );
		}
	}
	// Don't care about failure; just don't load anything.
}

void CConsole::SaveHistory()
{
	CStr buffer;
	std::deque<std::wstring>::iterator it;
	int line_count = 0;
	for( it = m_deqBufHistory.begin(); it != m_deqBufHistory.end(); ++it )
	{
		if(line_count++ >= m_MaxHistoryLines)
			break;
		buffer = CStrW( *it ).ToUTF8() + "\n" + buffer;
	}
	vfs_store( m_sHistoryFile, (void*)buffer.c_str(), buffer.Length(), FILE_NO_AIO ); 
}

void CConsole::SendChatMessage(const wchar_t *szMessage)
{
	if (g_NetClient || g_NetServer)
	{
		CChatMessage *msg=new CChatMessage();
		msg->m_Recipient = PS_CHAT_RCP_ALL;
		msg->m_Message = szMessage;
		if (g_NetClient)
			g_NetClient->Push(msg);
		else
		{
			msg->m_Sender=g_NetServer->GetServerPlayerName();
			ReceivedChatMessage(msg->m_Sender.c_str(), msg->m_Message.c_str());
			g_NetServer->Broadcast(msg);
		}
	}
}

void CConsole::ReceivedChatMessage(const wchar_t *szSender, const wchar_t *szMessage)
{
	InsertMessage(L"%ls: %ls", szSender, szMessage);
}

#include "sdl.h"

extern CConsole* g_Console;

extern void Die(int err, const wchar_t* fmt, ...);

int conInputHandler(const SDL_Event* ev)
{
	if( ev->type == SDL_HOTKEYDOWN )
	{
		if( ev->user.code == HOTKEY_CONSOLE_TOGGLE )
		{
			g_Console->ToggleVisible();
			return( EV_HANDLED );
		}
		else if( ev->user.code == HOTKEY_CONSOLE_COPY )
		{
			clipboard_set( g_Console->GetBuffer() );
		}
		else if( ev->user.code == HOTKEY_CONSOLE_PASTE )
		{
			wchar_t* text = clipboard_get();
			if(text)
			{
				for(wchar_t* c = text; *c; c++)
					g_Console->InsertChar(0, *c);

				clipboard_free(text);
			}
		}
	}

	if( ev->type != SDL_KEYDOWN)
		return EV_PASS;

	SDLKey sym = ev->key.keysym.sym;
	
	if(!g_Console->IsActive())
		return EV_PASS;

	// Stop unprintable characters (ctrl+, alt+ and escape), 
	// also prevent ` and/or ~ appearing in console every time it's toggled.
	// MT: Is this safe? Does any valid character require a ctrl+ or alt+
	//     key combination?
	// SB: Not safe, really.. Swedish keyboards have {[]} on AltGr (Ctrl-Alt)
	//     for example, so I commented those tests.
	if( ( ev->key.keysym.sym != SDLK_ESCAPE ) &&
		/*!keys[SDLK_LCTRL] && !keys[SDLK_RCTRL] &&
		!keys[SDLK_LALT] && !keys[SDLK_RALT] &&*/
		!hotkeys[HOTKEY_CONSOLE_TOGGLE] ) 
		g_Console->InsertChar(sym, (wchar_t)ev->key.keysym.unicode );

	return EV_PASS;
}
