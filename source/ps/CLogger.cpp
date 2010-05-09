/* Copyright (C) 2009 Wildfire Games.
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
#include "lib/path_util.h"
#include "lib/utf8.h"
#include "lib/sysdep/sysdep.h"

#include <ctime>
#include <ostream>

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

	*m_MainLog << L"<p>" << message << L"</p>\n";
	m_MainLog->flush();
	
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
