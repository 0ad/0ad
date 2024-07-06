/* Copyright (C) 2024 Wildfire Games.
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

#include <deque>
#include <fmt/printf.h>
#include <fstream>
#include <mutex>
#include <string>
#include <sstream>

class CCanvas2D;

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
	class ScopedReplacement;

	enum ELogMethod
	{
		Normal,
		Error,
		Warning
	};

	CLogger(std::ostream& mainLog, std::ostream& interestingLog, const bool useDebugPrintf);

	~CLogger();

	// Functions to write different message types (Errors and warnings are placed
	// both in mainLog and intrestingLog.)
	void WriteMessage(const char* message, bool doRender);
	void WriteError  (const char* message);
	void WriteWarning(const char* message);

	// Render recent log messages onto the screen
	void Render(CCanvas2D& canvas);

private:
	void Init();

	void PushRenderMessage(ELogMethod method, const char* message);

	// Delete old timed-out entries from the list of text to render
	void CleanupRenderQueue();

	// the output streams
	std::ostream& m_MainLog;
	std::ostream& m_InterestingLog;

	// whether errors should be reported via debug_printf (default)
	// or suppressed (for tests that intentionally trigger errors)
	bool m_UseDebugPrintf;

	// vars to hold message counts
	int m_NumberOfMessages{0};
	int m_NumberOfErrors{0};
	int m_NumberOfWarnings{0};

	// Used for Render()
	struct RenderedMessage
	{
		ELogMethod method;
		double time;
		std::string message;
	};
	std::deque<RenderedMessage> m_RenderMessages;

	// The logger might be constructed too early to allow us to call timer_Time(),
	// so we'll fill in the initial value later
	double m_RenderLastEraseTime{-1.0};

	// Lock for all state modified by logging commands
	std::mutex m_Mutex;
};

/**
 * Replaces `g_Logger` for as long as it's in scope.
 */
class CLogger::ScopedReplacement
{
public:
	ScopedReplacement(std::ostream& mainLog, std::ostream& interestingLog, const bool useDebugPrintf);

	ScopedReplacement(const ScopedReplacement&) = delete;
	ScopedReplacement& operator=(const ScopedReplacement&) = delete;
	ScopedReplacement(ScopedReplacement&&) = delete;
	ScopedReplacement& operator=(ScopedReplacement&&) = delete;
	~ScopedReplacement();

private:
	CLogger m_ThisLogger;
	CLogger* m_OldLogger;
};

/**
 * This is used in the engine to log the messages to the logfiles.
 */
class FileLogger
{
public:
	FileLogger();
private:
	std::ofstream m_MainLog;
	std::ofstream m_InterestingLog;
	CLogger::ScopedReplacement m_ScopedReplacement;
};

/**
 * Helper class for unit tests - captures all log output, and returns it as a
 * single string.
 */
class TestLogger
{
public:
	TestLogger();

	std::string GetOutput();
private:
	std::stringstream m_Stream;
	CLogger::ScopedReplacement m_ScopedReplacement;
};

/**
 * Helper class for unit tests - redirects all log output to stdout.
 */
class TestStdoutLogger
{
public:
	TestStdoutLogger();
private:
	CLogger::ScopedReplacement m_ScopedReplacement;
};

#endif
