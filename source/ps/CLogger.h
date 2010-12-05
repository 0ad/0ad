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

#include "ps/ThreadUtil.h"

class CLogger;
extern CLogger* g_Logger;


#define LOGMESSAGE g_Logger->LogMessage
#define LOGMESSAGERENDER g_Logger->LogMessageRender
#define LOGWARNING g_Logger->LogWarning
#define LOGERROR g_Logger->LogError

/**
 * Error/warning/message logging class.
 *
 * Thread-safety:
 * - Expected to be constructed/destructed in the main thread.
 * - The message logging functions may be called from any thread
 *   while the object is alive.
 */
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
	CLogger(std::ostream* mainLog, std::ostream* interestingLog, bool takeOwnership, bool useDebugPrintf);

	~CLogger();

	// Functions to write different message types (Errors and warnings are placed 
	// both in mainLog and intrestingLog.)
	void WriteMessage(const wchar_t* message, bool doRender);
	void WriteError  (const wchar_t* message);
	void WriteWarning(const wchar_t* message);
	
	// Functions to write a message, warning or error to file.
	void LogMessage(const wchar_t* fmt, ...) WPRINTF_ARGS(2);
	void LogMessageRender(const wchar_t* fmt, ...) WPRINTF_ARGS(2);
	void LogWarning(const wchar_t* fmt, ...) WPRINTF_ARGS(2);
	void LogError(const wchar_t* fmt, ...) WPRINTF_ARGS(2);

	// Render recent log messages onto the screen
	void Render();
	
private:
	void Init();

	void PushRenderMessage(ELogMethod method, const wchar_t* message);

	// Delete old timed-out entries from the list of text to render
	void CleanupRenderQueue();

	// the output streams
	std::ostream* m_MainLog;
	std::ostream* m_InterestingLog;
	bool m_OwnsStreams;

	// whether errors should be reported via debug_printf (default)
	// or suppressed (for tests that intentionally trigger errors)
	bool m_UseDebugPrintf;

	// vars to hold message counts
	int m_NumberOfMessages;
	int m_NumberOfErrors;
	int m_NumberOfWarnings;

	// Used for Render()
	struct RenderedMessage
	{
		ELogMethod method;
		double time;
		std::wstring message;
	};
	std::deque<RenderedMessage> m_RenderMessages;
	double m_RenderLastEraseTime;

	// Lock for all state modified by logging commands
	CMutex m_Mutex;
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
	std::stringstream m_Stream;
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
