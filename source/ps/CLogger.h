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

#ifndef INCLUDED_CLOGGER
#define INCLUDED_CLOGGER

#include <fstream>
#include <string>
#include <set>
#include <sstream>

class CLogger;
extern CLogger* g_Logger;

#define LOG (g_Logger->Log)
#define LOG_ONCE (g_Logger->LogOnce)

// Should become LOG_MESSAGE but this can only be changed once the LOG function is removed
// from all of the files. LOG_INFO, LOG_WARNING and LOG_ERROR are currently existing macros.

#define LOGMESSAGE g_Logger->LogMessage
#define LOGWARNING g_Logger->LogWarning
#define LOGERROR g_Logger->LogError

class CLogger
{
	NONCOPYABLE(CLogger);
public:
	enum ELogMethod
	{
		Normal,
		Error,
		Warning
	};

	// Default constructor - outputs to normal log files
	CLogger();

	// Special constructor (mostly for testing) - outputs to provided streams.
	// Can take ownership of streams and delete them in the destructor.
	CLogger(std::wostream* mainLog, std::wostream* interestingLog, bool takeOwnership, bool useDebugPrintf);

	~CLogger();

	// Functions to write different message types (Errors and warnings are placed 
	// both in mainLog and intrestingLog.)
	void WriteMessage(const wchar_t* message);
	void WriteError  (const wchar_t* message);
	void WriteWarning(const wchar_t* message);
	
	// Function to log stuff to file
	// -- This function has not been removed because the build would break.
	void Log(ELogMethod method, const wchar_t* category, const wchar_t* fmt, ...) WPRINTF_ARGS(4);
	// Similar to Log, but only outputs each message once no matter how many times it's called
	// -- This function has not been removed because the build would break.
	void LogOnce(ELogMethod method, const wchar_t* category, const wchar_t* fmt, ...) WPRINTF_ARGS(4);
	
	// Functions to write a message, warning or error to file.
	void LogMessage(const wchar_t* fmt, ...) WPRINTF_ARGS(2);
	void LogWarning(const wchar_t* fmt, ...) WPRINTF_ARGS(2);
	void LogError(const wchar_t* fmt, ...) WPRINTF_ARGS(2);

	// Render recent log messages onto the screen
	void Render();
	
private:
	void Init();

	// -- This function has not been removed because the build would break.
	void LogUsingMethod(ELogMethod method, const wchar_t* message);

	void PushRenderMessage(ELogMethod method, const wchar_t* message);

	// Delete old timed-out entries from the list of text to render
	void CleanupRenderQueue();

	// the output streams
	std::wostream* m_MainLog;
	std::wostream* m_InterestingLog;
	bool m_OwnsStreams;

	// whether errors should be reported via debug_printf (default)
	// or suppressed (for tests that intentionally trigger errors)
	bool m_UseDebugPrintf;

	// vars to hold message counts
	int m_NumberOfMessages;
	int m_NumberOfErrors;
	int m_NumberOfWarnings;

	// Used to remember LogOnce messages
	std::set<std::wstring> m_LoggedOnce;

	// Used for Render()
	struct RenderedMessage
	{
		ELogMethod method;
		double time;
		std::wstring message;
	};
	std::deque<RenderedMessage> m_RenderMessages;
	double m_RenderLastEraseTime;
};

/**
 * Helper class for unit tests - captures all log output while it is in scope,
 * and returns it as a single string.
 */
class TestLogger
{
	NONCOPYABLE(TestLogger);
public:
	TestLogger();
	~TestLogger();
	std::wstring GetOutput();
private:
	CLogger* m_OldLogger;
	std::wstringstream m_Stream;
};

/**
 * Helper class for unit tests - redirects all log output to stdout.
 */
class TestStdoutLogger
{
	NONCOPYABLE(TestStdoutLogger);
public:
	TestStdoutLogger();
	~TestStdoutLogger();
private:
	CLogger* m_OldLogger;
};

#endif
