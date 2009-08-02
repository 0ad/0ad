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
#include "lib/sysdep/sysdep.h"

#include <time.h>
#include <ostream>

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
	"<title>Pyrogenesis Log</title>\n"
	"<style>\n"
	"body { background: #eee; color: black; font-family: sans-serif; }\n"
	"p { background: white; margin: 3px 0 3px 0; }\n"
	".error { color: red; }\n"
	".warning { color: blue; }\n"
	"</style>\n"
	"<h2>0 A.D. ";

const char* html_header1 = "</h2>\n";

const char* html_footer = "";

CLogger::CLogger()
{
	fs::path mainlogPath(fs::path(psLogDir())/"mainlog.html");
	m_MainLog = new std::ofstream(mainlogPath.external_file_string().c_str(), std::ofstream::out | std::ofstream::trunc);

	fs::path interestinglogPath(fs::path(psLogDir())/"interestinglog.html");
	m_InterestingLog = new std::ofstream(interestinglogPath.external_file_string().c_str(), std::ofstream::out | std::ofstream::trunc);

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
	m_NumberOfMessages = 0;
	m_NumberOfErrors = 0;
	m_NumberOfWarnings = 0;
	
	//Write Headers for the HTML documents
	*m_MainLog << html_header0 << "Main log" << html_header1;
	
	//Write Headers for the HTML documents
	*m_InterestingLog << html_header0 << "Main log (warnings and errors only)" << html_header1;
}

CLogger::~CLogger ()
{
	char buffer[128];
	sprintf(buffer," with %d message(s), %d error(s) and %d warning(s).", \
						m_NumberOfMessages,m_NumberOfErrors,m_NumberOfWarnings);

	time_t t = time(NULL);
	struct tm* now = localtime(&t);
	//const char* months[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
	//CStr currentDate = CStr(months[now->tm_mon]) + " " + CStr(now->tm_mday) + " " + CStr(1900+now->tm_year);
	char currentDate[11];
	sprintf(currentDate, "%02d %02d %04d", now->tm_mon, now->tm_mday, (1900+now->tm_year));
	char currentTime[10];
	sprintf(currentTime, "%02d:%02d:%02d", now->tm_hour, now->tm_min, now->tm_sec);

	//Write closing text

	*m_MainLog << "<p>Engine exited successfully on " << currentDate;
	*m_MainLog << " at " << currentTime << buffer << "</p>\n";
	*m_MainLog << html_footer;
	
	*m_InterestingLog << "<p>Engine exited successfully on " << currentDate;
	*m_InterestingLog << " at " << currentTime << buffer << "</p>\n";
	*m_InterestingLog << html_footer;

	if (m_OwnsStreams)
	{
		delete m_InterestingLog;
		delete m_MainLog;
	}
}

void CLogger::WriteMessage(const char *message)
{
	++m_NumberOfMessages;

	*m_MainLog << "<p>" << message << "</p>\n";
	m_MainLog->flush();
	
}

void CLogger::WriteError(const char *message)
{
	++m_NumberOfErrors;
	if (m_UseDebugPrintf)
		debug_printf("ERROR: %s\n", message);

	if (g_Console) g_Console->InsertMessage(L"ERROR: %hs", message);
	*m_InterestingLog << "<p class=\"error\">ERROR: "<< message << "</p>\n";
	m_InterestingLog->flush();

	*m_MainLog << "<p class=\"error\">ERROR: "<< message << "</p>\n";
	m_MainLog->flush();
}

void CLogger::WriteWarning(const char *message)
{
	++m_NumberOfWarnings;

	if (g_Console) g_Console->InsertMessage(L"WARNING: %hs", message);
	*m_InterestingLog << "<p class=\"warning\">WARNING: "<< message << "</p>\n";
	m_InterestingLog->flush();

	*m_MainLog << "<p class=\"warning\">WARNING: "<< message << "</p>\n";
	m_MainLog->flush();
}

// Sends the message to the appropriate piece of code
// -- This function has not been removed because the build would break.
void CLogger::LogUsingMethod(ELogMethod method, const char* message)
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
void CLogger::Log(ELogMethod method, const char* UNUSED(category), const char *fmt, ...)
{
	va_list argp;
	char buffer[512];
	
	va_start(argp, fmt);
	if (sys_vsnprintf(buffer, sizeof(buffer), fmt, argp) == -1)
	{
		// Buffer too small - ensure the string is nicely terminated
		strcpy(buffer+sizeof(buffer)-4, "...");	// safe
	}
	va_end(argp);

	LogUsingMethod(method, buffer);
}

// -- This function has not been removed because the build would break.
void CLogger::LogOnce(ELogMethod method, const char* UNUSED(category), const char *fmt, ...)
{
	va_list argp;
	char buffer[512];

	va_start(argp, fmt);
	if (sys_vsnprintf(buffer, sizeof(buffer), fmt, argp) == -1)
	{
		// Buffer too small - ensure the string is nicely terminated
		strcpy(buffer+sizeof(buffer)-4, "...");	// safe
	}
	va_end(argp);

	std::string message (buffer);

	// If this message has already been logged, ignore it
	if (m_LoggedOnce.find(message) != m_LoggedOnce.end())
		return;

	// If not, mark it as having been logged and then log it
	m_LoggedOnce.insert(message);
	LogUsingMethod(method, buffer);
}

void CLogger::LogMessage(const char *fmt, ...)
{
	va_list argp;
	char buffer[512];
	
	va_start(argp, fmt);
	if (sys_vsnprintf(buffer, sizeof(buffer), fmt, argp) == -1)
	{
		// Buffer too small - ensure the string is nicely terminated
		strcpy(buffer+sizeof(buffer)-4, "...");	// safe
	}
	va_end(argp);

	WriteMessage(buffer);
}

void CLogger::LogWarning(const char *fmt, ...)
{
	va_list argp;
	char buffer[512];
	
	va_start(argp, fmt);
	if (sys_vsnprintf(buffer, sizeof(buffer), fmt, argp) == -1)
	{
		// Buffer too small - ensure the string is nicely terminated
		strcpy(buffer+sizeof(buffer)-4, "...");	// safe
	}
	va_end(argp);

	WriteWarning(buffer);
}

void CLogger::LogError(const char *fmt, ...)
{
	va_list argp;
	char buffer[512];
	
	va_start(argp, fmt);
	if (sys_vsnprintf(buffer, sizeof(buffer), fmt, argp) == -1)
	{
		// Buffer too small - ensure the string is nicely terminated
		strcpy(buffer+sizeof(buffer)-4, "...");	// safe
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

std::string TestLogger::GetOutput()
{
	return m_Stream.str();
}
