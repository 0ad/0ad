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

#ifndef INCLUDED_OS_PATH
#define INCLUDED_OS_PATH

#include "lib/path.h"

// rationale:
// users are responsible for ensuring the path doesn't contain any forbidden
// characters (including any code points < 0x00 or >= 0x100 on anything but Windows)
typedef Path OsPath;

#if OS_WIN

static inline const Path::String& OsString(const OsPath& path)
{
	return path.string();
}

#else

static inline std::string OsString(const OsPath& path)
{
	const Path::String& wstring = path.string();
	std::string string(wstring.length(), '\0');
	for(size_t i = 0; i < wstring.length(); i++)
	{
		ENSURE((unsigned)wstring[i] <= (unsigned)UCHAR_MAX);
		string[i] = (char)wstring[i];
	}
	return string;
}
#endif


#endif	// #ifndef INCLUDED_OS_PATH
