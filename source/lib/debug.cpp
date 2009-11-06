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

/*
 * platform-independent debug support code.
 */

#include "precompiled.h"
#include "debug.h"

#include <cstdarg>
#include <cstring>
#include <cstdio>

#include "app_hooks.h"
#include "path_util.h"
#include "lib/allocators/allocators.h"	// page_aligned_alloc
#include "fnv_hash.h"
#include "lib/sysdep/cpu.h"	// cpu_CAS
#include "lib/sysdep/sysdep.h"

#if OS_WIN
#include "lib/sysdep/os/win/wdbg_heap.h"
#endif


ERROR_ASSOCIATE(ERR::SYM_NO_STACK_FRAMES_FOUND, L"No stack frames found", -1);
ERROR_ASSOCIATE(ERR::SYM_UNRETRIEVABLE_STATIC, L"Value unretrievable (stored in external module)", -1);
ERROR_ASSOCIATE(ERR::SYM_UNRETRIEVABLE, L"Value unretrievable", -1);
ERROR_ASSOCIATE(ERR::SYM_TYPE_INFO_UNAVAILABLE, L"Error getting type_info", -1);
ERROR_ASSOCIATE(ERR::SYM_INTERNAL_ERROR, L"Exception raised while processing a symbol", -1);
ERROR_ASSOCIATE(ERR::SYM_UNSUPPORTED, L"Symbol type not (fully) supported", -1);
ERROR_ASSOCIATE(ERR::SYM_CHILD_NOT_FOUND, L"Symbol does not have the given child", -1);
ERROR_ASSOCIATE(ERR::SYM_NESTING_LIMIT, L"Symbol nesting too deep or infinite recursion", -1);
ERROR_ASSOCIATE(ERR::SYM_SINGLE_SYMBOL_LIMIT, L"Symbol has produced too much output", -1);
ERROR_ASSOCIATE(INFO::SYM_SUPPRESS_OUTPUT, L"Symbol was suppressed", -1);


// needed when writing crashlog
static const size_t LOG_CHARS = 16384;
wchar_t debug_log[LOG_CHARS];
wchar_t* debug_log_pos = debug_log;

// write to memory buffer (fast)
void debug_wprintf_mem(const wchar_t* fmt, ...)
{
	const ssize_t charsLeft = (ssize_t)(LOG_CHARS - (debug_log_pos-debug_log));
	debug_assert(charsLeft >= 0);

	// potentially not enough room for the new string; throw away the
	// older half of the log. we still protect against overflow below.
	if(charsLeft < 512)
	{
		const size_t copySize = sizeof(wchar_t) * LOG_CHARS/2;
		wchar_t* const middle = &debug_log[LOG_CHARS/2];
		cpu_memcpy(debug_log, middle, copySize);
		memset(middle, 0, copySize);
		debug_log_pos -= LOG_CHARS/2;	// don't assign middle (may leave gap)
	}

	// write into buffer (in-place)
	va_list args;
	va_start(args, fmt);
	int len = vswprintf(debug_log_pos, charsLeft-2, fmt, args);

	va_end(args);
	debug_log_pos += len+2;
	wcscpy_s(debug_log_pos-2, 3, L"\r\n");
}



// need to shoehorn printf-style variable params into
// the OutputDebugString call.
// - don't want to split into multiple calls - would add newlines to output.
// - fixing Win32 _vsnprintf to return # characters that would be written,
//   as required by C99, looks difficult and unnecessary. if any other code
//   needs that, implement GNU vasprintf.
// - fixed size buffers aren't nice, but much simpler than vasprintf-style
//   allocate+expand_until_it_fits. these calls are for quick debug output,
//   not loads of data, anyway.

// rationale: static data instead of std::set to allow setting at any time.
// we store FNV hash of tag strings for fast comparison; collisions are
// extremely unlikely and can only result in displaying more/less text.
static const size_t MAX_TAGS = 20;
static u32 tags[MAX_TAGS];
static size_t num_tags;

void debug_filter_add(const wchar_t* tag)
{
	const u32 hash = fnv_hash(tag, wcslen(tag)*sizeof(tag[0]));

	// make sure it isn't already in the list
	for(size_t i = 0; i < MAX_TAGS; i++)
		if(tags[i] == hash)
			return;

	// too many already?
	if(num_tags == MAX_TAGS)
	{
		debug_assert(0);	// increase MAX_TAGS
		return;
	}

	tags[num_tags++] = hash;
}

void debug_filter_remove(const wchar_t* tag)
{
	const u32 hash = fnv_hash(tag, wcslen(tag)*sizeof(tag[0]));

	for(size_t i = 0; i < MAX_TAGS; i++)
	{
		if(tags[i] == hash)	// found it
		{
			// replace with last element (avoid holes)
			tags[i] = tags[MAX_TAGS-1];
			num_tags--;

			// can only happen once, so we're done.
			return;
		}
	}
}

void debug_filter_clear()
{
	std::fill(tags, tags+MAX_TAGS, 0);
}

bool debug_filter_allows(const wchar_t* text)
{
	size_t i;
	for(i = 0; ; i++)
	{
		// no | found => no tag => should always be displayed
		if(text[i] == ' ' || text[i] == '\0')
			return true;
		if(text[i] == '|' && i != 0)
			break;
	}

	const u32 hash = fnv_hash(text, i*sizeof(text[0]));

	// check if entry allowing this tag is found
	for(i = 0; i < MAX_TAGS; i++)
		if(tags[i] == hash)
			return true;

	return false;
}

// max # characters (including \0) output by debug_printf in one call.
const size_t DEBUG_PRINTF_MAX_CHARS = 1024;	// refer to wdbg.cpp!debug_vsprintf before changing this

#undef debug_printf	// allowing #defining it out

void debug_printf(const wchar_t* fmt, ...)
{
	wchar_t buf[DEBUG_PRINTF_MAX_CHARS]; buf[ARRAY_SIZE(buf)-1] = '\0';

	va_list ap;
	va_start(ap, fmt);
	const int numChars = vswprintf_s(buf, DEBUG_PRINTF_MAX_CHARS, fmt, ap);
	debug_assert(numChars >= 0);
	va_end(ap);

	if(debug_filter_allows(buf))
		debug_puts(buf);
}


//-----------------------------------------------------------------------------

LibError debug_WriteCrashlog(const wchar_t* text)
{
	// (avoid infinite recursion and/or reentering this function if it
	// fails/reports an error)
	enum State
	{
		IDLE,
		BUSY,
		FAILED
	};
	// note: the initial state is IDLE. we rely on zero-init because
	// initializing local static objects from constants may happen when
	// this is first called, which isn't thread-safe. (see C++ 6.7.4)
	cassert(IDLE == 0);
	static volatile uintptr_t state;

	if(!cpu_CAS(&state, IDLE, BUSY))
		return ERR::REENTERED;	// NOWARN

	FILE* f;
	fs::wpath pathname = ah_get_log_dir()/L"crashlog.txt";
	errno_t err = _wfopen_s(&f, pathname.string().c_str(), L"w");
	if(err != 0)
	{
		state = FAILED;	// must come before DEBUG_DISPLAY_ERROR
		DEBUG_DISPLAY_ERROR(L"Unable to open crashlog.txt for writing (please ensure the log directory is writable)");
		return ERR::FAIL;	// NOWARN (the above text is more helpful than a generic error code)
	}

	fputwc(0xFEFF, f);	// BOM
	fwprintf(f, L"%ls\n", text);
	fwprintf(f, L"\n\n====================================\n\n");

	// allow user to bundle whatever information they want
	ah_bundle_logs(f);

	fwprintf(f, L"Last known activity:\n\n %ls\n", debug_log);

	fclose(f);
	state = IDLE;
	return INFO::OK;
}


//-----------------------------------------------------------------------------
// error message
//-----------------------------------------------------------------------------

// (NB: this may appear obscene, but deep stack traces have been
// observed to take up > 256 KiB)
static const size_t messageSize = 512*KiB;

void debug_FreeErrorMessage(ErrorMessageMem* emm)
{
	page_aligned_free(emm->pa_mem, messageSize);
}


// a stream with printf-style varargs and the possibility of
// writing directly to the output buffer.
class PrintfWriter
{
public:
	PrintfWriter(wchar_t* buf, size_t maxChars)
		: m_pos(buf), m_charsLeft(maxChars)
	{
	}

	bool operator()(const wchar_t* fmt, ...)
	{
		va_list ap;
		va_start(ap, fmt);
		const int len = vswprintf_s(m_pos, m_charsLeft, fmt, ap);
		va_end(ap);
		if(len < 0)
			return false;
		m_pos += len;
		m_charsLeft -= len;
		return true;
	}

	wchar_t* Position() const
	{
		return m_pos;
	}

	size_t CharsLeft() const
	{
		return m_charsLeft;
	}

	void CountAddedChars()
	{
		const size_t len = wcslen(m_pos);
		m_pos += len;
		m_charsLeft -= len;
	}

private:
	wchar_t* m_pos;
	size_t m_charsLeft;
};


// split out of debug_DisplayError because it's used by the self-test.
const wchar_t* debug_BuildErrorMessage(
	const wchar_t* description,
	const wchar_t* filename, int line, const char* func,
	void* context, const wchar_t* lastFuncToSkip,
	ErrorMessageMem* emm)
{
	// rationale: see ErrorMessageMem
	emm->pa_mem = page_aligned_alloc(messageSize);
	wchar_t* const buf = (wchar_t*)emm->pa_mem;
	if(!buf)
		return L"(insufficient memory to generate error message)";
	PrintfWriter writer(buf, messageSize / sizeof(wchar_t));

	// header
	if(!writer(
		L"%ls\r\n"
		L"Location: %ls:%d (%hs)\r\n"
		L"\r\n"
		L"Call stack:\r\n"
		L"\r\n",
		description, filename, line, func
	))
	{
fail:
		return L"(error while formatting error message)";
	}

	// append stack trace
	LibError ret = debug_DumpStack(writer.Position(), writer.CharsLeft(), context, lastFuncToSkip);
	if(ret == ERR::REENTERED)
	{
		if(!writer(
			L"While generating an error report, we encountered a second "
			L"problem. Please be sure to report both this and the subsequent "
			L"error messages."
		))
			goto fail;
	}
	else if(ret != INFO::OK)
	{
		wchar_t description_buf[100] = {'?'};
		if(!writer(
			L"(error while dumping stack: %ls)",
			error_description_r(ret, description_buf, ARRAY_SIZE(description_buf))
		))
			goto fail;
	}
	else	// success
	{
		writer.CountAddedChars();
	}

	// append OS error (just in case it happens to be relevant -
	// it's usually still set from unrelated operations)
	wchar_t description_buf[100] = L"?";
	LibError errno_equiv = LibError_from_errno(false);
	if(errno_equiv != ERR::FAIL)	// meaningful translation
		error_description_r(errno_equiv, description_buf, ARRAY_SIZE(description_buf));
	wchar_t os_error[100] = L"?";
	sys_error_description_r(0, os_error, ARRAY_SIZE(os_error));
	if(!writer(
		L"\r\n"
		L"errno = %d (%ls)\r\n"
		L"OS error = %ls\r\n",
		errno, description_buf, os_error
	))
		goto fail;

	return buf;
}


//-----------------------------------------------------------------------------
// display error messages
//-----------------------------------------------------------------------------

// translates and displays the given strings in a dialog.
// this is typically only used when debug_DisplayError has failed or
// is unavailable because that function is much more capable.
// implemented via sys_display_msg; see documentation there.
void debug_DisplayMessage(const wchar_t* caption, const wchar_t* msg)
{
	sys_display_msg(ah_translate(caption), ah_translate(msg));
}


// when an error has come up and user clicks Exit, we don't want any further
// errors (e.g. caused by atexit handlers) to come up, possibly causing an
// infinite loop. hiding errors isn't good, but we assume that whoever clicked
// exit really doesn't want to see any more messages.
static bool isExiting;

// this logic is applicable to any type of error. special cases such as
// suppressing certain expected WARN_ERRs are done there.
static bool ShouldSuppressError(u8* suppress)
{
	if(isExiting)
		return true;

	if(!suppress)
		return false;

	if(*suppress == DEBUG_SUPPRESS)
		return true;

	return false;
}

static ErrorReaction CallDisplayError(const wchar_t* text, size_t flags)
{
	// first try app hook implementation
	ErrorReaction er = ah_display_error(text, flags);
	// .. it's only a stub: default to normal implementation
	if(er == ER_NOT_IMPLEMENTED)
		er = sys_display_error(text, flags);

	return er;
}

static ErrorReaction PerformErrorReaction(ErrorReaction er, size_t flags, u8* suppress)
{
	const bool shouldHandleBreak = (flags & DE_MANUAL_BREAK) == 0;

	switch(er)
	{
	case ER_BREAK:
		// handle "break" request unless the caller wants to (doing so here
		// instead of within the dlgproc yields a correct call stack)
		if(shouldHandleBreak)
		{
			debug_break();
			er = ER_CONTINUE;
		}
		break;

	case ER_SUPPRESS:
		*suppress = DEBUG_SUPPRESS;
		er = ER_CONTINUE;
		break;

	case ER_EXIT:
		isExiting = true;	// see declaration

#if OS_WIN
		// prevent (slow) heap reporting since we're exiting abnormally and
		// thus probably leaking like a sieve.
		wdbg_heap_Enable(false);
#endif

		exit(EXIT_FAILURE);
	}

	return er;
}

ErrorReaction debug_DisplayError(const wchar_t* description,
	size_t flags, void* context, const wchar_t* lastFuncToSkip,
	const wchar_t* pathname, int line, const char* func,
	u8* suppress)
{
	// "suppressing" this error means doing nothing and returning ER_CONTINUE.
	if(ShouldSuppressError(suppress))
		return ER_CONTINUE;

	// fix up params
	// .. translate
	description = ah_translate(description);
	// .. caller supports a suppress flag; set the corresponding flag so that
	//    the error display implementation enables the Suppress option.
	if(suppress)
		flags |= DE_ALLOW_SUPPRESS;
	// .. deal with incomplete file/line info
	if(!pathname || pathname[0] == '\0')
		pathname = L"unknown";
	if(line <= 0)
		line = 0;
	if(!func || func[0] == '\0')
		func = "?";
	// .. _FILE__ evaluates to the full path (albeit without drive letter)
	//    which is rather long. we only display the base name for clarity.
	const wchar_t* filename = path_name_only(pathname);

	// display in output window; double-click will navigate to error location.
	debug_printf(L"%ls(%d): %ls\n", filename, line, description);

	ErrorMessageMem emm;
	const wchar_t* text = debug_BuildErrorMessage(description, filename, line, func, context, lastFuncToSkip, &emm);

	(void)debug_WriteCrashlog(text);
	ErrorReaction er = CallDisplayError(text, flags);

	// note: debug_break-ing here to make sure the app doesn't continue
	// running is no longer necessary. debug_DisplayError now determines our
	// window handle and is modal.

	// must happen before PerformErrorReaction because that may exit.
	debug_FreeErrorMessage(&emm);

	return PerformErrorReaction(er, flags, suppress);
}


// strobe indicating expectedError is valid and the next error should be
// compared against that / skipped if equal to it.
// set/reset via cpu_CAS for thread-safety (hence uintptr_t).
static uintptr_t isExpectedErrorValid;
static LibError expectedError;

void debug_SkipNextError(LibError err)
{
	if(cpu_CAS(&isExpectedErrorValid, 0, 1))
		expectedError = err;
	else
		debug_assert(0);	// internal error: concurrent attempt to skip assert/error

}

static bool ShouldSkipThisError(LibError err)
{
	// (compare before resetting strobe - expectedError may change afterwards)
	bool isExpected = (expectedError == err);
	// (use cpu_CAS to ensure only one error is skipped)
	if(cpu_CAS(&isExpectedErrorValid, 1, 0))
	{
		debug_assert(isExpected);
		return isExpected;
	}

	return false;
}

ErrorReaction debug_OnError(LibError err, u8* suppress, const wchar_t* file, int line, const char* func)
{
	if(ShouldSkipThisError(err))
		return ER_CONTINUE;

	void* context = 0;
	const wchar_t* lastFuncToSkip = L"debug_OnError";
	wchar_t buf[400];
	wchar_t err_buf[200]; error_description_r(err, err_buf, ARRAY_SIZE(err_buf));
	swprintf_s(buf, ARRAY_SIZE(buf), L"Function call failed: return value was %ld (%ls)", err, err_buf);
	return debug_DisplayError(buf, DE_MANUAL_BREAK, context, lastFuncToSkip, file,line,func, suppress);
}


void debug_SkipNextAssertion()
{
	// to share code between assert and error skip mechanism, we treat the
	// former as an error.
	debug_SkipNextError(ERR::ASSERTION_FAILED);
}


static bool ShouldSkipThisAssertion()
{
	return ShouldSkipThisError(ERR::ASSERTION_FAILED);
}

ErrorReaction debug_OnAssertionFailure(const wchar_t* expr, u8* suppress, const wchar_t* file, int line, const char* func)
{
	if(ShouldSkipThisAssertion())
		return ER_CONTINUE;
	void* context = 0;
	const std::wstring lastFuncToSkip = L"debug_OnAssertionFailure";
	wchar_t buf[400];
	swprintf_s(buf, ARRAY_SIZE(buf), L"Assertion failed: \"%ls\"", expr);
	return debug_DisplayError(buf, DE_MANUAL_BREAK, context, lastFuncToSkip.c_str(), file,line,func, suppress);
}
