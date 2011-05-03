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
#include "lib/timer.h"
#include "lib/utf8.h"
#include "lib/res/graphics/unifont.h"
#include "lib/sysdep/sysdep.h"
#include "ps/Font.h"

#include <ctime>
#include <iostream>

// Disable "'boost::algorithm::detail::is_classifiedF' : assignment operator could not be generated"
// and "find_format_store.hpp(74) : warning C4100: 'Input' : unreferenced formal parameter"
#if MSC_VERSION
#pragma warning(disable:4512)
#pragma warning(disable:4100)
#endif

#include <boost/algorithm/string/replace.hpp>

static const double RENDER_TIMEOUT = 10.0; // seconds before messages are deleted
static const double RENDER_TIMEOUT_RATE = 10.0; // number of timed-out messages deleted per second
static const size_t RENDER_LIMIT = 20; // maximum messages on screen at once

static const size_t BUFFER_SIZE = 1024;

extern int g_xres, g_yres;

// Set up a default logger that throws everything away, because that's
// better than crashing. (This is particularly useful for unit tests which
// don't care about any log output.)
struct BlackHoleStreamBuf : public std::streambuf
{
} blackHoleStreamBuf;
std::ostream blackHoleStream(&blackHoleStreamBuf);
CLogger nullLogger(&blackHoleStream, &blackHoleStream, false, true);

CLogger* g_Logger = &nullLogger;

const char* html_header0 =
	"<!DOCTYPE html>\n"
	"<meta charset=\"utf-8\">\n"
	"<title>Pyrogenesis Log</title>\n"
	"<style>"
	"body { background: #eee; color: black; font-family: sans-serif; } "
	"p { background: white; margin: 3px 0 3px 0; } "
	".error { color: red; } "
	".warning { color: blue; }"
	"</style>\n"
	"<h2>0 A.D. ";

const char* html_header1 = "</h2>\n";

CLogger::CLogger()
{
	OsPath mainlogPath(psLogDir()/"mainlog.html");
	m_MainLog = new std::ofstream(OsString(mainlogPath).c_str(), std::ofstream::out | std::ofstream::trunc);

	OsPath interestinglogPath(psLogDir()/"interestinglog.html");
	m_InterestingLog = new std::ofstream(OsString(interestinglogPath).c_str(), std::ofstream::out | std::ofstream::trunc);

	m_OwnsStreams = true;
	m_UseDebugPrintf = true;

	Init();
}

CLogger::CLogger(std::ostream* mainLog, std::ostream* interestingLog, bool takeOwnership, bool useDebugPrintf)
{
	m_MainLog = mainLog;
	m_InterestingLog = interestingLog;
	m_OwnsStreams = takeOwnership;
	m_UseDebugPrintf = useDebugPrintf;

	Init();
}

void CLogger::Init()
{
	m_RenderLastEraseTime = -1.0;
	// this is called too early to allow us to call timer_Time(),
	// so we'll fill in the initial value later

	m_NumberOfMessages = 0;
	m_NumberOfErrors = 0;
	m_NumberOfWarnings = 0;
	
	//Write Headers for the HTML documents
	*m_MainLog << html_header0 << "Main log" << html_header1;

	//Write Headers for the HTML documents
	*m_InterestingLog << html_header0 << "Main log (warnings and errors only)" << html_header1;
}

CLogger::~CLogger()
{
	char buffer[128];
	sprintf_s(buffer, ARRAY_SIZE(buffer), " with %d message(s), %d error(s) and %d warning(s).", m_NumberOfMessages,m_NumberOfErrors,m_NumberOfWarnings);

	time_t t = time(NULL);
	struct tm* now = localtime(&t);
	char currentDate[17];
	sprintf_s(currentDate, ARRAY_SIZE(currentDate), "%04d-%02d-%02d", 1900+now->tm_year, 1+now->tm_mon, now->tm_mday);
	char currentTime[10];
	sprintf_s(currentTime, ARRAY_SIZE(currentTime), "%02d:%02d:%02d", now->tm_hour, now->tm_min, now->tm_sec);

	//Write closing text

	*m_MainLog << "<p>Engine exited successfully on " << currentDate;
	*m_MainLog << " at " << currentTime << buffer << "</p>\n";
	
	*m_InterestingLog << "<p>Engine exited successfully on " << currentDate;
	*m_InterestingLog << " at " << currentTime << buffer << "</p>\n";

	if (m_OwnsStreams)
	{
		SAFE_DELETE(m_InterestingLog);
		SAFE_DELETE(m_MainLog);
	}
}

static std::string ToHTML(const wchar_t* message)
{
	Status err;
	std::string cmessage = utf8_from_wstring(message, &err);
	boost::algorithm::replace_all(cmessage, "&", "&amp;");
	boost::algorithm::replace_all(cmessage, "<", "&lt;");
	return cmessage;
}

void CLogger::WriteMessage(const wchar_t* message, bool doRender = false)
{
	std::string cmessage = ToHTML(message);

	CScopeLock lock(m_Mutex);

	++m_NumberOfMessages;
//	if (m_UseDebugPrintf)
//		debug_printf(L"MESSAGE: %ls\n", message);

	*m_MainLog << "<p>" << cmessage << "</p>\n";
	m_MainLog->flush();
	
	if (doRender)
	{
		if (g_Console) g_Console->InsertMessage(L"INFO: %ls", message);

		PushRenderMessage(Normal, message);
	}
}

void CLogger::WriteError(const wchar_t* message)
{
	std::string cmessage = ToHTML(message);

	CScopeLock lock(m_Mutex);

	++m_NumberOfErrors;
	if (m_UseDebugPrintf)
		debug_printf(L"ERROR: %ls\n", message);

	if (g_Console) g_Console->InsertMessage(L"ERROR: %ls", message);
	*m_InterestingLog << "<p class=\"error\">ERROR: " << cmessage << "</p>\n";
	m_InterestingLog->flush();

	*m_MainLog << "<p class=\"error\">ERROR: " << cmessage << "</p>\n";
	m_MainLog->flush();

	PushRenderMessage(Error, message);
}

void CLogger::WriteWarning(const wchar_t* message)
{
	std::string cmessage = ToHTML(message);

	CScopeLock lock(m_Mutex);

	++m_NumberOfWarnings;
	if (m_UseDebugPrintf)
		debug_printf(L"WARNING: %ls\n", message);

	if (g_Console) g_Console->InsertMessage(L"WARNING: %ls", message);
	*m_InterestingLog << "<p class=\"warning\">WARNING: " << cmessage << "</p>\n";
	m_InterestingLog->flush();

	*m_MainLog << "<p class=\"warning\">WARNING: " << cmessage << "</p>\n";
	m_MainLog->flush();

	PushRenderMessage(Warning, message);
}

void CLogger::LogMessage(const wchar_t* fmt, ...)
{
	va_list argp;
	wchar_t buffer[BUFFER_SIZE] = {0};
	
	va_start(argp, fmt);
	if (sys_vswprintf(buffer, ARRAY_SIZE(buffer), fmt, argp) == -1)
	{
		// Buffer too small - ensure the string is nicely terminated
		wcscpy_s(buffer+ARRAY_SIZE(buffer)-4, 4, L"...");
	}
	va_end(argp);

	WriteMessage(buffer);
}

void CLogger::LogMessageRender(const wchar_t* fmt, ...)
{
	va_list argp;
	wchar_t buffer[BUFFER_SIZE] = {0};
	
	va_start(argp, fmt);
	if (sys_vswprintf(buffer, ARRAY_SIZE(buffer), fmt, argp) == -1)
	{
		// Buffer too small - ensure the string is nicely terminated
		wcscpy_s(buffer+ARRAY_SIZE(buffer)-4, 4, L"...");
	}
	va_end(argp);

	WriteMessage(buffer, true);
}

void CLogger::LogWarning(const wchar_t* fmt, ...)
{
	va_list argp;
	wchar_t buffer[BUFFER_SIZE] = {0};
	
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
	wchar_t buffer[BUFFER_SIZE] = {0};
	
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

	// (Lock must come after loading the CFont, since that might log error messages
	// and attempt to lock the mutex recursively which is forbidden)
	CScopeLock lock(m_Mutex);

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
	CScopeLock lock(m_Mutex);

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
	Status err;
	return wstring_from_utf8(m_Stream.str(), &err);
}

TestStdoutLogger::TestStdoutLogger()
{
	m_OldLogger = g_Logger;
	g_Logger = new CLogger(&std::cout, &blackHoleStream, false, false);
}

TestStdoutLogger::~TestStdoutLogger()
{
	delete g_Logger;
	g_Logger = m_OldLogger;
}
