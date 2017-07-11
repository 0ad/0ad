/* Copyright (C) 2010 Wildfire Games.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * platform-independent debug support code.
 */

#include "precompiled.h"
#include "lib/debug.h"

#include <cstdarg>
#include <cstring>
#include <cstdio>

#include "lib/alignment.h"
#include "lib/app_hooks.h"
#include "lib/fnv_hash.h"
#include "lib/sysdep/vm.h"
#include "lib/sysdep/cpu.h"	// cpu_CAS
#include "lib/sysdep/sysdep.h"

#if OS_WIN
# include "lib/sysdep/os/win/wdbg_heap.h"
#endif

static const StatusDefinition debugStatusDefinitions[] = {
	{ ERR::SYM_NO_STACK_FRAMES_FOUND, L"No stack frames found" },
	{ ERR::SYM_UNRETRIEVABLE_STATIC, L"Value unretrievable (stored in external module)" },
	{ ERR::SYM_UNRETRIEVABLE, L"Value unretrievable" },
	{ ERR::SYM_TYPE_INFO_UNAVAILABLE, L"Error getting type_info" },
	{ ERR::SYM_INTERNAL_ERROR, L"Exception raised while processing a symbol" },
	{ ERR::SYM_UNSUPPORTED, L"Symbol type not (fully) supported" },
	{ ERR::SYM_CHILD_NOT_FOUND, L"Symbol does not have the given child" },
	{ ERR::SYM_NESTING_LIMIT, L"Symbol nesting too deep or infinite recursion" },
	{ ERR::SYM_SINGLE_SYMBOL_LIMIT, L"Symbol has produced too much output" },
	{ INFO::SYM_SUPPRESS_OUTPUT, L"Symbol was suppressed" }
};
STATUS_ADD_DEFINITIONS(debugStatusDefinitions);


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

void debug_filter_add(const char* tag)
{
	const u32 hash = fnv_hash(tag, strlen(tag)*sizeof(tag[0]));

	// make sure it isn't already in the list
	for(size_t i = 0; i < MAX_TAGS; i++)
		if(tags[i] == hash)
			return;

	// too many already?
	if(num_tags == MAX_TAGS)
	{
		DEBUG_WARN_ERR(ERR::LOGIC);	// increase MAX_TAGS
		return;
	}

	tags[num_tags++] = hash;
}

void debug_filter_remove(const char* tag)
{
	const u32 hash = fnv_hash(tag, strlen(tag)*sizeof(tag[0]));

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

bool debug_filter_allows(const char* text)
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

#undef debug_printf	// allowing #defining it out
void debug_printf(const char* fmt, ...)
{
	char buf[16384];

	va_list ap;
	va_start(ap, fmt);
	const int numChars = vsprintf_s(buf, ARRAY_SIZE(buf), fmt, ap);
	if (numChars < 0)
		debug_break();  // poor man's assert - avoid infinite loop because ENSURE also uses debug_printf
	va_end(ap);

	debug_puts_filtered(buf);
}

void debug_puts_filtered(const char* text)
{
	if(debug_filter_allows(text))
		debug_puts(text);
}


//-----------------------------------------------------------------------------

Status debug_WriteCrashlog(const wchar_t* text)
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
	static volatile intptr_t state;

	if(!cpu_CAS(&state, IDLE, BUSY))
		return ERR::REENTERED;	// NOWARN

	OsPath pathname = ah_get_log_dir()/"crashlog.txt";
	FILE* f = sys_OpenFile(pathname, "w");
	if(!f)
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
	vm::Free(emm->pa_mem, messageSize);
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

	bool operator()(const wchar_t* fmt, ...) WPRINTF_ARGS(2)
	{
		va_list ap;
		va_start(ap, fmt);
		const int len = vswprintf(m_pos, m_charsLeft, fmt, ap);
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
	// retrieve errno (might be relevant) before doing anything else
	// that might overwrite it.
	wchar_t description_buf[100] = L"?";
	wchar_t os_error[100] = L"?";
	Status errno_equiv = StatusFromErrno();	// NOWARN
	if(errno_equiv != ERR::FAIL)	// meaningful translation
		StatusDescription(errno_equiv, description_buf, ARRAY_SIZE(description_buf));
	sys_StatusDescription(0, os_error, ARRAY_SIZE(os_error));

	// rationale: see ErrorMessageMem
	emm->pa_mem = vm::Allocate(messageSize);
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
	Status ret = debug_DumpStack(writer.Position(), writer.CharsLeft(), context, lastFuncToSkip);
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
		wchar_t error_buf[100] = {'?'};
		if(!writer(
			L"(error while dumping stack: %ls)",
			StatusDescription(ret, error_buf, ARRAY_SIZE(error_buf))
		))
			goto fail;
	}
	else	// success
	{
		writer.CountAddedChars();
	}

	// append errno
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
static atomic_bool isExiting;

// this logic is applicable to any type of error. special cases such as
// suppressing certain expected WARN_ERRs are done there.
static bool ShouldSuppressError(atomic_bool* suppress)
{
	if(isExiting)
		return true;

	if(!suppress)
		return false;

	if(*suppress == DEBUG_SUPPRESS)
		return true;

	return false;
}

static ErrorReactionInternal CallDisplayError(const wchar_t* text, size_t flags)
{
	// first try app hook implementation
	ErrorReactionInternal er = ah_display_error(text, flags);
	// .. it's only a stub: default to normal implementation
	if(er == ERI_NOT_IMPLEMENTED)
		er = sys_display_error(text, flags);

	return er;
}

static ErrorReaction PerformErrorReaction(ErrorReactionInternal er, size_t flags, atomic_bool* suppress)
{
	const bool shouldHandleBreak = (flags & DE_MANUAL_BREAK) == 0;

	switch(er)
	{
	case ERI_CONTINUE:
		return ER_CONTINUE;

	case ERI_BREAK:
		// handle "break" request unless the caller wants to (doing so here
		// instead of within the dlgproc yields a correct call stack)
		if(shouldHandleBreak)
		{
			debug_break();
			return ER_CONTINUE;
		}
		else
			return ER_BREAK;

	case ERI_SUPPRESS:
		(void)cpu_CAS(suppress, 0, DEBUG_SUPPRESS);
		return ER_CONTINUE;

	case ERI_EXIT:
		isExiting = 1;	// see declaration
		COMPILER_FENCE;

#if OS_WIN
		// prevent (slow) heap reporting since we're exiting abnormally and
		// thus probably leaking like a sieve.
		wdbg_heap_Enable(false);
#endif

		exit(EXIT_FAILURE);

	case ERI_NOT_IMPLEMENTED:
	default:
		debug_break();	// not expected to be reached
		return ER_CONTINUE;
	}
}

ErrorReaction debug_DisplayError(const wchar_t* description,
	size_t flags, void* context, const wchar_t* lastFuncToSkip,
	const wchar_t* pathname, int line, const char* func,
	atomic_bool* suppress)
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

	if(flags & DE_NO_DEBUG_INFO)
	{
		// in non-debug-info mode, simply display the given description
		// and then return immediately
		ErrorReactionInternal er = CallDisplayError(description, flags);
		return PerformErrorReaction(er, flags, suppress);
	}

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
	debug_printf("%s(%d): %s\n", utf8_from_wstring(filename).c_str(), line, utf8_from_wstring(description).c_str());

	ErrorMessageMem emm;
	const wchar_t* text = debug_BuildErrorMessage(description, filename, line, func, context, lastFuncToSkip, &emm);

	(void)debug_WriteCrashlog(text);
	ErrorReactionInternal er = CallDisplayError(text, flags);

	// note: debug_break-ing here to make sure the app doesn't continue
	// running is no longer necessary. debug_DisplayError now determines our
	// window handle and is modal.

	// must happen before PerformErrorReaction because that may exit.
	debug_FreeErrorMessage(&emm);

	return PerformErrorReaction(er, flags, suppress);
}


// is errorToSkip valid? (also guarantees mutual exclusion)
enum SkipStatus
{
	INVALID, VALID, BUSY
};
static intptr_t skipStatus = INVALID;
static Status errorToSkip;
static size_t numSkipped;

void debug_SkipErrors(Status err)
{
	if(cpu_CAS(&skipStatus, INVALID, BUSY))
	{
		errorToSkip = err;
		numSkipped = 0;
		COMPILER_FENCE;
		skipStatus = VALID;	// linearization point
	}
	else
		DEBUG_WARN_ERR(ERR::REENTERED);
}

size_t debug_StopSkippingErrors()
{
	if(cpu_CAS(&skipStatus, VALID, BUSY))
	{
		const size_t ret = numSkipped;
		COMPILER_FENCE;
		skipStatus = INVALID;	// linearization point
		return ret;
	}
	else
	{
		DEBUG_WARN_ERR(ERR::REENTERED);
		return 0;
	}
}

static bool ShouldSkipError(Status err)
{
	if(cpu_CAS(&skipStatus, VALID, BUSY))
	{
		numSkipped++;
		const bool ret = (err == errorToSkip);
		COMPILER_FENCE;
		skipStatus = VALID;
		return ret;
	}
	return false;
}


ErrorReaction debug_OnError(Status err, atomic_bool* suppress, const wchar_t* file, int line, const char* func)
{
	CACHE_ALIGNED(u8) context[DEBUG_CONTEXT_SIZE];
	(void)debug_CaptureContext(context);

	if(ShouldSkipError(err))
		return ER_CONTINUE;

	const wchar_t* lastFuncToSkip = L"debug_OnError";
	wchar_t buf[400];
	wchar_t err_buf[200]; StatusDescription(err, err_buf, ARRAY_SIZE(err_buf));
	swprintf_s(buf, ARRAY_SIZE(buf), L"Function call failed: return value was %lld (%ls)", (long long)err, err_buf);
	return debug_DisplayError(buf, DE_MANUAL_BREAK, context, lastFuncToSkip, file,line,func, suppress);
}


ErrorReaction debug_OnAssertionFailure(const wchar_t* expr, atomic_bool* suppress, const wchar_t* file, int line, const char* func)
{
	CACHE_ALIGNED(u8) context[DEBUG_CONTEXT_SIZE];
	(void)debug_CaptureContext(context);

	const std::wstring lastFuncToSkip = L"debug_OnAssertionFailure";
	wchar_t buf[400];
	swprintf_s(buf, ARRAY_SIZE(buf), L"Assertion failed: \"%ls\"", expr);
	return debug_DisplayError(buf, DE_MANUAL_BREAK, context, lastFuncToSkip.c_str(), file,line,func, suppress);
}
