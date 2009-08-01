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

#include "precompiled.h"

#include "Pyrogenesis.h"
#include "ps/i18n.h"

#include "lib/sysdep/sysdep.h"
#include "lib/path_util.h"
#include "lib/svn_revision.h"

DEFINE_ERROR(PS_OK, "OK");
DEFINE_ERROR(PS_FAIL, "Fail");


static const wchar_t* translate_no_mem = L"(no mem)";

// overrides ah_translate. registered in GameSetup.cpp
const wchar_t* psTranslate(const wchar_t* text)
{
	// make sure i18n system is (already|still) initialized.
	if(g_CurrentLocale)
	{
		// be prepared for this to fail, because translation potentially
		// involves script code and the JS context might be corrupted.
		try
		{
			CStrW ret = I18n::translate(text);
			const wchar_t* ret_dup = wcsdup(ret.c_str());
			return ret_dup? ret_dup : translate_no_mem;
		}
		catch(...)
		{
		}
	}

	// i18n not available: at least try and return the text (unchanged)
	const wchar_t* ret_dup = wcsdup(text);
	return ret_dup? ret_dup : translate_no_mem;
}

void psTranslateFree(const wchar_t* text)
{
	if(text != translate_no_mem)
		free((void*)text);
}


// convert contents of file <in_filename> from char to wchar_t and
// append to <out> file.
static void AppendAsciiFile(FILE* out, const char* in_filename)
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

// for user convenience, bundle all logs into this file:
void psBundleLogs(FILE* f)
{
	fwprintf(f, L"SVN Revision: %s\n\n", svn_revision);

	fwprintf(f, L"System info:\n\n");
	fs::path path1(psLogPath()/"system_info.txt");
	AppendAsciiFile(f, path1.external_file_string().c_str());
	fwprintf(f, L"\n\n====================================\n\n");

	fwprintf(f, L"Main log:\n\n");
	fs::path path2(psLogPath()/"mainlog.html");
	AppendAsciiFile(f, path2.external_file_string().c_str());
	fwprintf(f, L"\n\n====================================\n\n");
}


const char* psGetLogDir()
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


fs::path psLogPath()
{
	char exePathname[PATH_MAX];
	(void)sys_get_executable_name(exePathname, ARRAY_SIZE(exePathname));
	path_strip_fn(exePathname);
	return fs::path(exePathname)/"../logs/";
}
