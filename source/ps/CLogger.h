/* Copyright (C) 2019 Wildfire Games.
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
#include <mutex>
#include <string>
#include <set>
#include <sstream>

#include "third_party/cppformat/format.h"

class CLogger;
extern CLogger* g_Logger;


#define LOGMESSAGE(...)       g_Logger->WriteMessage(fmt::sprintf(__VA_ARGS__).c_str(), false)
#define LOGMESSAGERENDER(...) g_Logger->WriteMessage(fmt::sprintf(__VA_ARGS__).c_str(), true)
#define LOGWARNING(...)       g_Logger->WriteWarning(fmt::sprintf(__VA_ARGS__).c_str())
#define LOGERROR(...)         g_Logger->WriteError  (fmt::sprintf(__VA_ARGS__).c_str())

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
	void WriteMessage(const char* message, bool doRender);
	void WriteError  (const char* message);
	void WriteWarning(const char* message);

	// Render recent log messages onto the screen
	void Render();

private:
	void Init();

	void PushRenderMessage(ELogMethod method, const char* message);

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
		std::string message;
	};
	std::deque<RenderedMessage> m_RenderMessages;
	double m_RenderLastEraseTime;

	// Lock for all state modified by logging commands
	std::mutex m_Mutex;
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
	std::string GetOutput();
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
