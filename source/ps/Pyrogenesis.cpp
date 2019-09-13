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

#include "precompiled.h"

#include <cstdio>

#include "Pyrogenesis.h"

#include "lib/sysdep/sysdep.h"
#include "lib/svn_revision.h"

const char* engine_version = "0.0.24";

// convert contents of file <in_filename> from char to wchar_t and
// append to <out> file.
static void AppendAsciiFile(FILE* out, const OsPath& pathname)
{
	FILE* in = sys_OpenFile(pathname, "rb");
	if (!in)
	{
		fwprintf(out, L"(unavailable)");
		return;
	}

	const size_t buf_size = 1024;
	char buf[buf_size+1]; // include space for trailing '\0'

	while (!feof(in))
	{
		size_t bytes_read = fread(buf, 1, buf_size, in);
		if (!bytes_read)
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
	fwprintf(f, L"Engine Version: %hs\n\n", engine_version);

	fwprintf(f, L"System info:\n\n");
	OsPath path1 = psLogDir()/"system_info.txt";
	AppendAsciiFile(f, path1);
	fwprintf(f, L"\n\n====================================\n\n");

	fwprintf(f, L"Main log:\n\n");
	OsPath path2 = psLogDir()/"mainlog.html";
	AppendAsciiFile(f, path2);
	fwprintf(f, L"\n\n====================================\n\n");
}


static OsPath logDir;

void psSetLogDir(const OsPath& newLogDir)
{
	logDir = newLogDir;
}

const OsPath& psLogDir()
{
	return logDir;
}
