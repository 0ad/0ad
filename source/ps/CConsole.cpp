#include "CConsole.h"

CConsole::CConsole(float X, float Y, float W, float H):
					m_fX(X), m_fY(Y), m_fMaxWidth(W), m_fMaxHeight(H)
{
	m_fHeight = m_fMaxHeight;
	m_fWidth = m_fMaxWidth;

	m_bToggle = false;
	m_bVisible = false;

	m_szBuffer = (char*)malloc(sizeof(char) * BUFFER_SIZE);
	FlushBuffer();

	m_iMsgHistPos = 1;

	InsertMessage("[ 0 A.D. Console v0.11 ]   type \"\\info\" for help");
	InsertMessage("\0");
}


CConsole::~CConsole()
{
	m_mapFuncList.clear();
	m_deqMsgHistory.clear();
	m_deqBufHistory.clear();
	delete[] m_szBuffer;
}


void CConsole::FlushBuffer(void)
{
	/* Clear the buffer and set the cursor and length to 0 */
	memset(m_szBuffer, '\0', sizeof(char) * BUFFER_SIZE);
	m_iBufferPos = m_iBufferLength = 0;
}


char* CConsole::ToLower(const char* szMessage, uint iSize)
{
	uint L = (uint)strlen(szMessage);

	if (L <= 0) return NULL;

	if (iSize && iSize < L) L = iSize;

	char* szTemp = (char*)malloc(sizeof(char) * (L + 1));
	memset(szTemp, '\0', sizeof(char) * (L + 1));

    for(uint i = 0; i < L; i++)
		szTemp[i] = tolower(szMessage[i]);

	return szTemp;
}


char* CConsole::Trim(const char* szMessage, const char cChar, uint iSize)
{
	uint L = (uint)strlen(szMessage);

	/* return NULL if szMessage is invalid */
	if (L <= 0) return NULL;

	if (iSize && iSize < L) L = iSize;

	char szChar[1];
	szChar[0] = cChar;

	/* Find the first point at which szChar does not */
	/* exist in the message */
	uint iResult = (uint)strspn(szMessage, szChar);
	if (iResult <= 0) return (char*)szMessage;

	/* Create a temp buffer for the remaining characters */
	char* szTemp = (char*)malloc(sizeof(char) * (L - iResult + 1));
	memset(szTemp, '\0', sizeof(char) * (L - iResult + 1));

	/* Store the remaining characters in a temp buffer */
	for (uint i = iResult; i < L; i++)
		szTemp[i - iResult] = szMessage[i];

	L = (i - iResult);

	for (i = L; i >= 0; i--)
	{
		szTemp[i] = '\0';
		if (szTemp[i - 1] != cChar) break;
	}

	return szTemp;
}


void CConsole::RegisterFunc(fptr F, const char* szName)
{
	char* szTemp = (char*) malloc(sizeof(char) * (BUFFER_SIZE - 1));
	memset(szTemp, '\0', sizeof(char) * (BUFFER_SIZE - 1));

	szTemp = Trim(szName, (BUFFER_SIZE - 1));
	szTemp = ToLower(szTemp, (BUFFER_SIZE - 1));

	m_mapFuncList.insert(std::pair<char*, fptr>(szTemp, F));

	delete[] szTemp;
}

void CConsole::Update(const float DeltaTime)
{
	const float MoveSpeed = 10.0f;

	if (m_bToggle)
	{
		if (m_fHeight > m_fY)
			m_fHeight -= MoveSpeed * DeltaTime;
		else
			if (m_bVisible) m_bVisible = false;
	}
	else
	{
		if (!m_bVisible) m_bVisible = true;

		if (m_fHeight < m_fMaxHeight)
			m_fHeight += MoveSpeed * DeltaTime;
		else
			if (m_fHeight != m_fMaxHeight)
				m_fHeight = m_fMaxHeight;
	}
}

//Render Manager.
void CConsole::Render()
{
	if (!m_bVisible) return;

	glTranslatef(m_fX, m_fY, 0.0f); //Move to window position

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

			if (m_fHeight > FONT_HEIGHT + 4)
			{
				glVertex2f(0.0f,		FONT_HEIGHT + 4);
				glVertex2f(m_fWidth,	FONT_HEIGHT + 4);
			}
		glEnd();
	glPopMatrix();

	glEnable(GL_TEXTURE_2D);
}


void CConsole::DrawHistory(void) {
	int i = 1;
	
	std::deque<std::string>::iterator Iter; //History iterator

	glPushMatrix();
		glColor3f(1.0f, 1.0f, 1.0f); //Set color of text
		glTranslatef(9.0f, 0.0f, 0.0f); //move away from the border

		for (Iter = m_deqMsgHistory.begin();
			 Iter != m_deqMsgHistory.end()
				 && (((i - m_iMsgHistPos + 1) * FONT_HEIGHT) < (m_fHeight - FONT_HEIGHT));
			 Iter++)
		{
			if (i >= m_iMsgHistPos){
				glTranslatef(0.0f, FONT_HEIGHT, 0.0f);

				glPushMatrix();
					glprintf("%s", Iter->data());
				glPopMatrix();
			}

			i++;
		}
	glPopMatrix();
}

//Renders the buffer to the screen.
void CConsole::DrawBuffer(void)
{
	if (m_fHeight < FONT_HEIGHT) return;

	glPushMatrix();
		glColor3f(1.0f, 1.0f, 0.0f);
		glTranslatef(2.0f, 0.0f, 0);
		glprintf("]");

		glColor3f(1.0f, 1.0f, 1.0f);
		if (m_iBufferPos==0) DrawCursor();

		for (int i = 0; i < m_iBufferLength; i++){
				glprintf("%c", m_szBuffer[i]);
				if (m_iBufferPos-1==i) DrawCursor();
		}
	glPopMatrix();
}


void CConsole::DrawCursor(void)
{
	glPushMatrix();
		glprintf("_");
	glPopMatrix();
}


//Inserts a character into the buffer.
void CConsole::InsertChar(const int szChar)
{
	static int iHistoryPos = -1;

	if (szChar == '`')
	{
		m_bToggle = !m_bToggle;
		return;
	}

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

		case SDLK_LEFT:
			if (m_iBufferPos) m_iBufferPos--;
			return;

		case SDLK_RIGHT:
			if (m_iBufferPos != m_iBufferLength) m_iBufferPos++;
			return;

		/* BEGIN: Buffer History Lookup */
		case SDLK_UP:
			if (m_deqBufHistory.size() && iHistoryPos != m_deqBufHistory.size() - 1)
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
			if (m_iMsgHistPos != m_deqMsgHistory.size()) m_iMsgHistPos++;
			return;

		case SDLK_PAGEDOWN:
			if (m_iMsgHistPos != 1) m_iMsgHistPos--;
			return;
		/* END: Message History Lookup */

		default: //Insert a character
			if (IsFull()) return;
			if (!isprint(szChar)) return;

			if (IsEOB()) //are we at the end of the buffer?
				m_szBuffer[m_iBufferPos] = szChar; //cat char onto end
			else{ //we need to insert
				for(int i=m_iBufferLength; i>m_iBufferPos; i--)
					m_szBuffer[i] = m_szBuffer[i-1]; // move chars to right
				m_szBuffer[i] = szChar;
			}

			m_iBufferPos++;
			m_iBufferLength++;

			return;
	}
}


void CConsole::InsertMessage(const char* szMessage, ...)
{
	va_list args;
	char* szBuffer = (char*)malloc(sizeof(char)*(BUFFER_SIZE));

	va_start(args, szMessage);
		_vsnprintf(szBuffer, BUFFER_SIZE, szMessage, args);
	va_end(args);

	m_deqMsgHistory.push_front(szBuffer);
}


void CConsole::SetBuffer(const char* szMessage, ...)
{
	va_list args;
	char* szBuffer = (char*)malloc(sizeof(char)*(BUFFER_SIZE));

	va_start(args, szMessage);
		_vsnprintf(szBuffer, BUFFER_SIZE, szMessage, args);
	va_end(args);

	FlushBuffer();

	strncpy(m_szBuffer, szMessage, BUFFER_SIZE);
	m_iBufferLength = m_iBufferPos = (int)strlen(m_szBuffer);
}


void CConsole::ProcessBuffer(const char* szLine){
	if (szLine == NULL) return;
	if (strlen(szLine) <= 0) return;

	m_deqBufHistory.push_front(szLine);

	char* szCommand = (char*)malloc(sizeof(char));
    memset(szCommand, '\0', sizeof(char));

	std::map<std::string, fptr>::iterator Iter;

	if (szLine[0] == '\\'){
		sscanf(szLine, "\\%s", szCommand);
		szCommand = Trim(szCommand);
		szCommand = ToLower(szCommand);

		if (!strcmp(szCommand, "info")){
			InsertMessage("\0");
			InsertMessage("[Information]");
			InsertMessage("   -View commands \"\\commands\"");
			InsertMessage("   -Call command \"\\<command>\"");
			InsertMessage("   -Say \"<string>\"");
			InsertMessage("\0");
		}else if (!strcmp(szCommand, "commands")){
			InsertMessage("\0");
			InsertMessage("[Commands]");

			if (!m_mapFuncList.size()) InsertMessage("   (none registered)");

			for (Iter = m_mapFuncList.begin(); Iter != m_mapFuncList.end(); Iter++)
				InsertMessage("   \\%s", Iter->first.data());

			InsertMessage("\0");
		}else{
			try{
				m_mapFuncList.find(szCommand)->second();
			}catch(...){
				InsertMessage("unknown command <%s>", szCommand);
			}
		}
	}
	else InsertMessage("<say>: %s", szLine);

	delete[] szCommand;
}


#include "sdl.h"

extern CConsole* g_Console;
static bool console_active = false;

bool conInputHandler(const SDL_Event& ev)
{
	if(ev.type != SDL_KEYDOWN)
		return false;

	const int c = ev.key.keysym.sym;
	if(c == SDLK_F1)
	{
		console_active = !console_active;
//		g_Console->Toggle();
	}

	if(console_active)
	{
		g_Console->InsertChar(c);
		return true;
	}
	else
		return false;
}
