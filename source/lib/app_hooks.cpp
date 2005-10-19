// Hooks to allow application-specific behavior
// Copyright (c) 2005 Jan Wassenberg
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// Contact info:
//   Jan.Wassenberg@stud.uni-karlsruhe.de
//   http://www.stud.uni-karlsruhe.de/~urkt/

#include "precompiled.h"

#include <string.h>
#include <stdio.h>

#include "detect.h"
#include "res/graphics/ogl_tex.h"
#include "res/file/file.h"

#include "app_hooks.h"


//-----------------------------------------------------------------------------
// default implementations
//-----------------------------------------------------------------------------

static void override_gl_upload_caps()
{
	if(gfx_card[0] == '\0')
		debug_warn("get_gfx_info must be called before ogl_tex_upload");

	if(!strcmp(gfx_card, "S3 SuperSavage/IXC 1014"))
	{
		if(strstr(gfx_drv_ver, "ssicdnt.dll (2.60.115)"))
			ogl_tex_override(OGL_TEX_S3TC, OGL_TEX_DISABLE);
	}
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

static void bundle_logs(FILE* f)
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


// TODO: leaks memory returned by wcsdup
static const wchar_t* translate(const wchar_t* text)
{
#if HAVE_I18N
	// make sure i18n system is (already|still) initialized.
	if(g_CurrentLocale)
	{
		// be prepared for this to fail, because translation potentially
		// involves script code and the JS context might be corrupted.
#if OS_WIN
		__try
#endif
		{
			const wchar_t* text2 = wcsdup(I18n::translate(text).c_str());
			// only overwrite if wcsdup succeeded, i.e. not out of memory.
			if(text2)
				text = text2;
		}
#if OS_WIN
		__except(EXCEPTION_EXECUTE_HANDLER)
#endif
		{
		}
	}
#endif

	return text;
}


static void log(const wchar_t* text)
{
	wprintf(text);
}


//-----------------------------------------------------------------------------

// contains the current set of hooks. starts with the stub values and
// may be changed via set_app_hooks.
//
// rationale: we don't ever need to switch "hook sets", so one global struct
// is fine. by always having one defined, we also avoid having to check
// if anything was registered yet.
static AppHooks ah =
{
#define FUNC(ret, name, params, param_names, call_prefix) name,
#include "app_hooks.h"
#undef FUNC

	// int dummy; used to terminate list, since last entry ended with ','.
	0
};

// register the specified hook function pointers. any of them that
// are non-zero override the previous function pointer value
// (these default to the stub hooks which are functional but basic).
void set_app_hooks(AppHooks* ah_)
{
	debug_assert(ah_);
	ONCE_NOT(debug_warn("app hooks already set"));

	// override members in <ah> if they are non-zero in <ah_>
	// (otherwise, we stick with the defaults set above)
#define FUNC(ret, name, params, param_names, call_prefix) if(ah_->name) ah.name = ah_->name;
#include "app_hooks.h"
#undef FUNC
}

// trampolines used by lib code; they call the hooks or fall back to the
// default implementation if not set.
#define FUNC(ret, name, params, param_names, call_prefix) inline ret ah_##name params { call_prefix ah.name param_names; }
#include "app_hooks.h"
#undef FUNC
