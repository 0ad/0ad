/* Copyright (c) 2015 Wildfire Games
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
 * hooks to allow customization / app-specific behavior.
 */

#include "precompiled.h"
#include "lib/app_hooks.h"

#include "lib/sysdep/sysdep.h"

#include <cstdio>

//-----------------------------------------------------------------------------
// default implementations
//-----------------------------------------------------------------------------

static void def_override_gl_upload_caps()
{
}


static const OsPath& def_get_log_dir()
{
	static OsPath logDir;
	if(logDir.empty())
		logDir = sys_ExecutablePathname().Parent();
	return logDir;
}


static void def_bundle_logs(FILE* UNUSED(f))
{
}


static const wchar_t* def_translate(const wchar_t* text)
{
	return text;
}


static void def_translate_free(const wchar_t* UNUSED(text))
{
	// no-op - translate() doesn't own the pointer.
}


static void def_log(const wchar_t* text)
{
#if ICC_VERSION
#pragma warning(push)
#pragma warning(disable:181)    // "invalid printf conversion" - but wchar_t* and %ls are legit
#endif
	printf("%ls", text); // must not use wprintf, since stdout on Unix is byte-oriented
#if ICC_VERSION
#pragma warning(pop)
#endif
}


static ErrorReactionInternal def_display_error(const wchar_t* UNUSED(text), size_t UNUSED(flags))
{
	return ERI_NOT_IMPLEMENTED;
}


//-----------------------------------------------------------------------------

// contains the current set of hooks. starts with the default values and
// may be changed via app_hooks_update.
//
// rationale: we don't ever need to switch "hook sets", so one global struct
// is fine. by always having one defined, we also avoid the trampolines
// having to check whether their function pointer is valid.
static AppHooks ah =
{
	def_override_gl_upload_caps,
	def_get_log_dir,
	def_bundle_logs,
	def_translate,
	def_translate_free,
	def_log,
	def_display_error
};

// separate copy of ah; used to determine if a particular hook has been
// redefined. the additional storage needed is negligible and this is
// easier than comparing each value against its corresponding def_ value.
static AppHooks default_ah = ah;

// register the specified hook function pointers. any of them that
// are non-zero override the previous function pointer value
// (these default to the stub hooks which are functional but basic).
void app_hooks_update(AppHooks* new_ah)
{
	ENSURE(new_ah);

#define OVERRIDE_IF_NONZERO(HOOKNAME) if(new_ah->HOOKNAME) ah.HOOKNAME = new_ah->HOOKNAME;
	OVERRIDE_IF_NONZERO(override_gl_upload_caps)
	OVERRIDE_IF_NONZERO(get_log_dir)
	OVERRIDE_IF_NONZERO(bundle_logs)
	OVERRIDE_IF_NONZERO(translate)
	OVERRIDE_IF_NONZERO(translate_free)
	OVERRIDE_IF_NONZERO(log)
	OVERRIDE_IF_NONZERO(display_error)
#undef OVERRIDE_IF_NONZERO
}

bool app_hook_was_redefined(size_t offset_in_struct)
{
	const u8* ah_bytes = (const u8*)&ah;
	const u8* default_ah_bytes = (const u8*)&default_ah;
	typedef void(*FP)();	// a bit safer than comparing void* pointers
	if(*(FP)(ah_bytes+offset_in_struct) != *(FP)(default_ah_bytes+offset_in_struct))
		return true;
	return false;
}


//-----------------------------------------------------------------------------
// trampoline implementations
// (boilerplate code; hides details of how to call the app hook)
//-----------------------------------------------------------------------------

void ah_override_gl_upload_caps()
{
	if(ah.override_gl_upload_caps)
		ah.override_gl_upload_caps();
}

const OsPath& ah_get_log_dir()
{
	return ah.get_log_dir();
}

void ah_bundle_logs(FILE* f)
{
	ah.bundle_logs(f);
}

const wchar_t* ah_translate(const wchar_t* text)
{
	return ah.translate(text);
}

void ah_translate_free(const wchar_t* text)
{
	ah.translate_free(text);
}

void ah_log(const wchar_t* text)
{
	ah.log(text);
}

ErrorReactionInternal ah_display_error(const wchar_t* text, size_t flags)
{
	return ah.display_error(text, flags);
}
