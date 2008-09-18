/**
 * =========================================================================
 * File        : app_hooks.cpp
 * Project     : 0 A.D.
 * Description : hooks to allow customization / app-specific behavior.
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#include "precompiled.h"
#include "app_hooks.h"

#include "lib/path_util.h"
#include "lib/sysdep/sysdep.h"


//-----------------------------------------------------------------------------
// default implementations
//-----------------------------------------------------------------------------

static void def_override_gl_upload_caps()
{
}


static const char* def_get_log_dir()
{
	static char N_log_dir[PATH_MAX];
	ONCE(\
		(void)sys_get_executable_name(N_log_dir, ARRAY_SIZE(N_log_dir));\
		/* strip app name (we only want its path) */\
		path_strip_fn(N_log_dir);\
	);
	return N_log_dir;
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
	wprintf(text);
}


static ErrorReaction def_display_error(const wchar_t* UNUSED(text), size_t UNUSED(flags))
{
	return ER_NOT_IMPLEMENTED;
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
	debug_assert(new_ah);

#define OVERRIDE_IF_NONZERO(HOOKNAME) if(new_ah->HOOKNAME) ah.HOOKNAME = new_ah->HOOKNAME;
	OVERRIDE_IF_NONZERO(override_gl_upload_caps)
	OVERRIDE_IF_NONZERO(get_log_dir)
	OVERRIDE_IF_NONZERO(bundle_logs)
	OVERRIDE_IF_NONZERO(translate)
	OVERRIDE_IF_NONZERO(translate_free)
	OVERRIDE_IF_NONZERO(log)
	OVERRIDE_IF_NONZERO(display_error)
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

void ah_override_gl_upload_caps(void)
{
	if(ah.override_gl_upload_caps)
		ah.override_gl_upload_caps();
}

const char* ah_get_log_dir(void)
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

ErrorReaction ah_display_error(const wchar_t* text, size_t flags)
{
	return ah.display_error(text, flags);
}
