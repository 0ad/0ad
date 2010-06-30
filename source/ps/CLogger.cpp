/* Copyright (C) 2010 Wildfire Games.
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

#include "CLogger.h"
#include "CConsole.h"
#include "ConfigDB.h"
#include "lib/ogl.h"
#include "lib/path_util.h"
#include "lib/timer.h"
#include "lib/utf8.h"
#include "lib/res/graphics/unifont.h"
#include "lib/sysdep/sysdep.h"
#include "ps/Font.h"

#include <ctime>
#include <ostream>

static const double RENDER_TIMEOUT = 10.0; // seconds before messages are deleted
static const double RENDER_TIMEOUT_RATE = 10.0; // number of timed-out messages deleted per second
static const size_t RENDER_LIMIT = 20; // maximum messages on screen at once

extern int g_xres, g_yres;

// Set up a default logger that throws everything away, because that's
// better than crashing. (This is particularly useful for unit tests which
// don't care about any log output.)
struct BlackHoleStreamBuf : public std::wstreambuf
{
} blackHoleStreamBuf;
std::wostream blackHoleStream(&blackHoleStreamBuf);
CLogger nullLogger(&blackHoleStream, &blackHoleStream, false, true);

CLogger* g_Logger = &nullLogger;

const wchar_t* html_header0 =
	L"<!DOCTYPE html>\n"
	L"<title>Pyrogenesis Log</title>\n"
	L"<style>\n"
	L"body { background: #eee; color: black; font-family: sans-serif; }\n"
	L"p { background: white; margin: 3px 0 3px 0; }\n"
	L".error { color: red; }\n"
	L".warning { color: blue; }\n"
	L"</style>\n"
	L"<h2>0 A.D. ";

const wchar_t* html_header1 = L"</h2>\n";

const wchar_t* html_footer = L"";

CLogger::CLogger()
{
	fs::wpath mainlogPath(psLogDir()/L"mainlog.html");
	m_MainLog = new std::wofstream(utf8_from_wstring(mainlogPath.string()).c_str(), std::ofstream::out | std::ofstream::trunc);

	fs::wpath interestinglogPath(psLogDir()/L"interestinglog.html");
	m_InterestingLog = new std::wofstream(utf8_from_wstring(interestinglogPath.string()).c_str(), std::ofstream::out | std::ofstream::trunc);

	m_OwnsStreams = true;
	m_UseDebugPrintf = true;

	Init();
}

CLogger::CLogger(std::wostream* mainLog, std::wostream* interestingLog, bool takeOwnership, bool useDebugPrintf)
{
	m_MainLog = mainLog;
	m_InterestingLog = interestingLog;
	m_OwnsStreams = takeOwnership;
	m_UseDebugPrintf = useDebugPrintf;

	m_RenderLastEraseTime = -1.0;
	// this is called too early to allow us to call timer_Time(),
	// so we'll fill in the initial value later

	Init();
}

void CLogger::Init()
{
	m_NumberOfMessages = 0;
	m_NumberOfErrors = 0;
	m_NumberOfWarnings = 0;
	
	//Write Headers for the HTML documents
	*m_MainLog << html_header0 << L"Main log" << html_header1;
	
	//Write Headers for the HTML documents
	*m_InterestingLog << html_header0 << L"Main log (warnings and errors only)" << html_header1;
}

CLogger::~CLogger ()
{
	wchar_t buffer[128];
	swprintf_s(buffer, ARRAY_SIZE(buffer), L" with %d message(s), %d error(s) and %d warning(s).", m_NumberOfMessages,m_NumberOfErrors,m_NumberOfWarnings);

	time_t t = time(NULL);
	struct tm* now = localtime(&t);
	wchar_t currentDate[11];
	swprintf_s(currentDate, ARRAY_SIZE(currentDate), L"%02d %02d %04d", now->tm_mon, now->tm_mday, (1900+now->tm_year));
	wchar_t currentTime[10];
	swprintf_s(currentTime, ARRAY_SIZE(currentTime), L"%02d:%02d:%02d", now->tm_hour, now->tm_min, now->tm_sec);

	//Write closing text

	*m_MainLog << L"<p>Engine exited successfully on " << currentDate;
	*m_MainLog << L" at " << currentTime << buffer << L"</p>\n";
	*m_MainLog << html_footer;
	
	*m_InterestingLog << L"<p>Engine exited successfully on " << currentDate;
	*m_InterestingLog << L" at " << currentTime << buffer << L"</p>\n";
	*m_InterestingLog << html_footer;

	if (m_OwnsStreams)
	{
		SAFE_DELETE(m_InterestingLog);
		SAFE_DELETE(m_MainLog);
	}
}

void CLogger::WriteMessage(const wchar_t* message)
{
	++m_NumberOfMessages;
//	if (m_UseDebugPrintf)
//		debug_printf(L"MESSAGE: %ls\n", message);

	*m_MainLog << L"<p>" << message << L"</p>\n";
	m_MainLog->flush();
	
	// Don't do this since it results in too much noise:
//	PushRenderMessage(Normal, message);
}

void CLogger::WriteError(const wchar_t* message)
{
	++m_NumberOfErrors;
	if (m_UseDebugPrintf)
		debug_printf(L"ERROR: %ls\n", message);

	if (g_Console) g_Console->InsertMessage(L"ERROR: %ls", message);
	*m_InterestingLog << L"<p class=\"error\">ERROR: "<< message << L"</p>\n";
	m_InterestingLog->flush();

	*m_MainLog << L"<p class=\"error\">ERROR: "<< message << L"</p>\n";
	m_MainLog->flush();

	PushRenderMessage(Error, message);
}

void CLogger::WriteWarning(const wchar_t* message)
{
	++m_NumberOfWarnings;
	if (m_UseDebugPrintf)
		debug_printf(L"WARNING: %ls\n", message);

	if (g_Console) g_Console->InsertMessage(L"WARNING: %ls", message);
	*m_InterestingLog << L"<p class=\"warning\">WARNING: "<< message << L"</p>\n";
	m_InterestingLog->flush();

	*m_MainLog << L"<p class=\"warning\">WARNING: "<< message << L"</p>\n";
	m_MainLog->flush();

	PushRenderMessage(Warning, message);
}

// Sends the message to the appropriate piece of code
// -- This function has not been removed because the build would break.
void CLogger::LogUsingMethod(ELogMethod method, const wchar_t* message)
{
	if(method == Normal)
		WriteMessage(message);
	else if(method == Error)
		WriteError(message);
	else if(method == Warning)
		WriteWarning(message);
	else
		WriteMessage(message);
}

// -- This function has not been removed because the build would break.
void CLogger::Log(ELogMethod method, const wchar_t* UNUSED(category), const wchar_t* fmt, ...)
{
	va_list argp;
	wchar_t buffer[512];
	
	va_start(argp, fmt);
	if (sys_vswprintf(buffer, ARRAY_SIZE(buffer), fmt, argp) == -1)
	{
		// Buffer too small - ensure the string is nicely terminated
		wcscpy_s(buffer+ARRAY_SIZE(buffer)-4, 4, L"...");
	}
	va_end(argp);

	LogUsingMethod(method, buffer);
}

// -- This function has not been removed because the build would break.
void CLogger::LogOnce(ELogMethod method, const wchar_t* UNUSED(category), const wchar_t* fmt, ...)
{
	va_list argp;
	wchar_t buffer[512];

	va_start(argp, fmt);
	if (sys_vswprintf(buffer, ARRAY_SIZE(buffer), fmt, argp) == -1)
	{
		// Buffer too small - ensure the string is nicely terminated
		wcscpy_s(buffer+ARRAY_SIZE(buffer)-4, 4, L"...");
	}
	va_end(argp);

	std::wstring message(buffer);

	// If this message has already been logged, ignore it
	if (m_LoggedOnce.find(message) != m_LoggedOnce.end())
		return;

	// If not, mark it as having been logged and then log it
	m_LoggedOnce.insert(message);
	LogUsingMethod(method, buffer);
}

void CLogger::LogMessage(const wchar_t* fmt, ...)
{
	va_list argp;
	wchar_t buffer[512];
	
	va_start(argp, fmt);
	if (sys_vswprintf(buffer, ARRAY_SIZE(buffer), fmt, argp) == -1)
	{
		// Buffer too small - ensure the string is nicely terminated
		wcscpy_s(buffer+ARRAY_SIZE(buffer)-4, 4, L"...");
	}
	va_end(argp);

	WriteMessage(buffer);
}

void CLogger::LogWarning(const wchar_t* fmt, ...)
{
	va_list argp;
	wchar_t buffer[512];
	
	va_start(argp, fmt);
	if (sys_vswprintf(buffer, ARRAY_SIZE(buffer), fmt, argp) == -1)
	{
		// Buffer too small - ensure the string is nicely terminated
		wcscpy_s(buffer+ARRAY_SIZE(buffer)-4, 4, L"...");
	}
	va_end(argp);

	WriteWarning(buffer);
}

void CLogger::LogError(const wchar_t* fmt, ...)
{
	va_list argp;
	wchar_t buffer[512];
	
	va_start(argp, fmt);
	if (sys_vswprintf(buffer, ARRAY_SIZE(buffer), fmt, argp) == -1)
	{
		// Buffer too small - ensure the string is nicely terminated
		wcscpy_s(buffer+ARRAY_SIZE(buffer)-4, 4, L"...");
	}
	va_end(argp);

	WriteError(buffer);
}

void CLogger::Render()
{
	CleanupRenderQueue();

	CFont font(L"mono-stroke-10");
	int lineSpacing = font.GetLineSpacing();
	font.Bind();

	glPushMatrix();

	glScalef(1.0f, -1.0f, 1.0f);
	glTranslatef(4.0f, 4.0f + (float)lineSpacing - g_yres, 0.0f);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	for (std::deque<RenderedMessage>::iterator it = m_RenderMessages.begin(); it != m_RenderMessages.end(); ++it)
	{
		const wchar_t* type;
		if (it->method == Normal)
		{
			type = L"info";
			glColor3f(0.0f, 0.8f, 0.0f);
		}
		else if (it->method == Warning)
		{
			type = L"warning";
			glColor3f(1.0f, 1.0f, 0.0f);
		}
		else
		{
			type = L"error";
			glColor3f(1.0f, 0.0f, 0.0f);
		}
		glPushMatrix();

		glwprintf(L"[%8.3f] %ls: ", it->time, type);
		// Display the actual message in white so it's more readable
		glColor3f(1.0f, 1.0f, 1.0f);
		glwprintf(L"%ls", it->message.c_str());

		glPopMatrix();
		glTranslatef(0.f, (float)lineSpacing, 0.f);
	}

	glDisable(GL_BLEND);

	glPopMatrix();
}

void CLogger::PushRenderMessage(ELogMethod method, const wchar_t* message)
{
	double now = timer_Time();

	// Add each message line separately
	const wchar_t* pos = message;
	const wchar_t* eol;
	while ((eol = wcschr(pos, L'\n')) != NULL)
	{
		if (eol != pos)
		{
			RenderedMessage r = { method, now, std::wstring(pos, eol) };
			m_RenderMessages.push_back(r);
		}
		pos = eol + 1;
	}
	// Add the last line, if we didn't end on a \n
	if (*pos != L'\0')
	{
		RenderedMessage r = { method, now, std::wstring(pos) };
		m_RenderMessages.push_back(r);
	}
}

void CLogger::CleanupRenderQueue()
{
	if (m_RenderMessages.empty())
		return;

	double now = timer_Time();

	// Initialise the timer on the first call (since we can't do it in the ctor)
	if (m_RenderLastEraseTime == -1.0)
		m_RenderLastEraseTime = now;

	// Delete old messages, approximately at the given rate limit (and at most one per frame)
	if (now - m_RenderLastEraseTime > 1.0/RENDER_TIMEOUT_RATE)
	{
		if (m_RenderMessages[0].time + RENDER_TIMEOUT < now)
		{
			m_RenderMessages.pop_front();
			m_RenderLastEraseTime = now;
		}
	}

	// If there's still too many then delete the oldest
	if (m_RenderMessages.size() > RENDER_LIMIT)
		m_RenderMessages.erase(m_RenderMessages.begin(), m_RenderMessages.end() - RENDER_LIMIT);
}

TestLogger::TestLogger()
{
	m_OldLogger = g_Logger;
	g_Logger = new CLogger(&m_Stream, &blackHoleStream, false, false);
}

TestLogger::~TestLogger()
{
	delete g_Logger;
	g_Logger = m_OldLogger;
}

std::wstring TestLogger::GetOutput()
{
	return m_Stream.str();
}

TestStdoutLogger::TestStdoutLogger()
{
	m_OldLogger = g_Logger;
	g_Logger = new CLogger(&std::wcout, &blackHoleStream, false, false);
}

TestStdoutLogger::~TestStdoutLogger()
{
	delete g_Logger;
	g_Logger = m_OldLogger;
}
