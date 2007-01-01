/**
 * =========================================================================
 * File        : app_hooks.cpp
 * Project     : 0 A.D.
 * Description : hooks to allow customization / app-specific behavior.
 *
 * @author Jan.Wassenberg@stud.uni-karlsruhe.de
 * =========================================================================
 */

/*
 * Copyright (c) 2005 Jan Wassenberg
 *
 * Redistribution and/or modification are also permitted under the
 * terms of the GNU General Public License as published by the
 * Free Software Foundation (version 2 or later, at your option).
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#include "precompiled.h"
#include "app_hooks.h"

#include <string.h>
#include <stdio.h>

#include "lib/path_util.h"
#include "lib/sysdep/gfx.h"
#include "lib/res/graphics/ogl_tex.h"
#include "lib/res/file/file.h"
#include "lib/res/file/vfs.h"


//-----------------------------------------------------------------------------
// default implementations
//-----------------------------------------------------------------------------

static void def_override_gl_upload_caps()
{
	if(gfx_card[0] == '\0')
		debug_warn("gfx_detect must be called before ogl_tex_upload");

	if(!strcmp(gfx_card, "S3 SuperSavage/IXC 1014"))
	{
		if(strstr(gfx_drv_ver, "ssicdnt.dll (2.60.115)"))
			ogl_tex_override(OGL_TEX_S3TC, OGL_TEX_DISABLE);
	}
}


static const char* def_get_log_dir()
{
	static char N_log_dir[PATH_MAX];
	ONCE(\
		char N_exe_name[PATH_MAX];\
		(void)sys_get_executable_name(N_exe_name, ARRAY_SIZE(N_exe_name));\
		/* strip app name (we only want its path) */\
		path_strip_fn(N_exe_name);\
		(void)path_append(N_log_dir, N_exe_name, "../logs/");
	);
	return N_log_dir;
}


// convert contents of file <in_filename> from char to wchar_t and
// append to <out> file.
static void cat_atow(FILE* out, const char* in_filename)
{
	FILE* in = fopen(in_filename, "rb");
	if(!in)
	{
		fwprintf(out, L"(unavailable)");
		return;
	}

	const size_t buf_size = 1024;
	char buf[buf_size+1]; // include space for trailing '\0'

	while(!feof(in))
	{
		size_t bytes_read = fread(buf, 1, buf_size, in);
		if(!bytes_read)
			break;
		buf[bytes_read] = 0;	// 0-terminate
		fwprintf(out, L"%hs", buf);
	}

	fclose(in);
}

static void def_bundle_logs(FILE* f)
{
	// for user convenience, bundle all logs into this file:
	char N_path[PATH_MAX];

	fwprintf(f, L"System info:\n\n");
	(void)file_make_full_native_path("../logs/system_info.txt", N_path);
	cat_atow(f, N_path);
	fwprintf(f, L"\n\n====================================\n\n");

	fwprintf(f, L"Main log:\n\n");
	(void)file_make_full_native_path("../logs/mainlog.html", N_path);
	cat_atow(f, N_path);
	fwprintf(f, L"\n\n====================================\n\n");
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


static ErrorReaction def_display_error(const wchar_t* UNUSED(text), uint UNUSED(flags))
{
	return ER_NOT_IMPLEMENTED;
}


//-----------------------------------------------------------------------------

// contains the current set of hooks. starts with the default values and
// may be changed via app_hooks_update.
//
// rationale: we don't ever need to switch "hook sets", so one global struct
// is fine. by always having one defined, we also avoid having to check
// if anything was registered yet.
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


//-----------------------------------------------------------------------------
// trampoline implementations
// (boilerplate code; hides details of how to call the app hook)
//-----------------------------------------------------------------------------

void ah_override_gl_upload_caps(void)
{
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

ErrorReaction ah_display_error(const wchar_t* text, uint flags)
{
	return ah.display_error(text, flags);
}
