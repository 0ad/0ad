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

#include <cstdio>

#include "Pyrogenesis.h"

#include "lib/sysdep/sysdep.h"
#include "lib/path_util.h"
#include "lib/svn_revision.h"

static const wchar_t* translate_no_mem = L"(no mem)";

// overrides ah_translate. registered in GameSetup.cpp
const wchar_t* psTranslate(const wchar_t* text)
{
	// TODO: implement this somehow

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
static void AppendAsciiFile(FILE* out, const NativePath& pathname)
{
	FILE* in;
	errno_t err = _wfopen_s(&in, pathname.c_str(), L"rb");
	if(err != 0)
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
	fwprintf(f, L"SVN Revision: %ls\n\n", svn_revision);

	fwprintf(f, L"System info:\n\n");
	NativePath path1 = Path::Join(psLogDir(), L"system_info.txt");
	AppendAsciiFile(f, path1);
	fwprintf(f, L"\n\n====================================\n\n");

	fwprintf(f, L"Main log:\n\n");
	NativePath path2 = Path::Join(psLogDir(), L"mainlog.html");
	AppendAsciiFile(f, path2);
	fwprintf(f, L"\n\n====================================\n\n");
}


static std::wstring logDir;

void psSetLogDir(const std::wstring& newLogDir)
{
	logDir = newLogDir;
}

const std::wstring& psLogDir()
{
	return logDir;
}
