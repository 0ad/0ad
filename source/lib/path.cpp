/* Copyright (c) 2010 Wildfire Games
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
 * helper functions for path strings.
 */

#include "precompiled.h"
#include "lib/path.h"

#include <cstring>
#include <cerrno>

static const StatusDefinition pathStatusDefinitions[] = {
	{ ERR::PATH_CHARACTER_ILLEGAL, L"illegal path character" },
	{ ERR::PATH_CHARACTER_UNSAFE, L"unsafe path character" },
	{ ERR::PATH_NOT_FOUND, L"path not found" },
	{ ERR::PATH_MIXED_SEPARATORS, L"path contains both slash and backslash separators" }
};
STATUS_ADD_DEFINITIONS(pathStatusDefinitions);


static bool path_is_dir_sep(wchar_t c)
{
	if(c == '/' || c == '\\')
		return true;
	return false;
}

// is s2 a subpath of s1, or vice versa?
// (equal counts as subpath)
bool path_is_subpath(const wchar_t* s1, const wchar_t* s2)
{
	// make sure s1 is the shorter string
	if(wcslen(s1) > wcslen(s2))
		std::swap(s1, s2);

	wchar_t c1 = 0, last_c1, c2;
	for(;;)
	{
		last_c1 = c1;
		c1 = *s1++, c2 = *s2++;

		// end of s1 reached:
		if(c1 == '\0')
		{
			// s1 matched s2 up until:
			if((c2 == '\0') ||	// its end (i.e. they're equal length) OR
				path_is_dir_sep(c2) ||		// start of next component OR
				path_is_dir_sep(last_c1))	// ", but both have a trailing slash
				// => is subpath
				return true;
		}

		// mismatch => is not subpath
		if(c1 != c2)
			return false;
	}
}


//-----------------------------------------------------------------------------

// return pointer to the name component within path (i.e. skips over all
// characters up to the last dir separator, if any).
const wchar_t* path_name_only(const wchar_t* path)
{
	const wchar_t* slash1 = wcsrchr(path, '/');
	const wchar_t* slash2 = wcsrchr(path, '\\');
	// neither present, it's a filename only
	if(!slash1 && !slash2)
		return path;

	// return name, i.e. component after the last slash
	const wchar_t* name = std::max(slash1, slash2)+1;
	return name;
}


/*static*/ Status Path::Validate(String::value_type c)
{
	if(c < 32)
		return ERR::PATH_CHARACTER_UNSAFE;

#if !OS_WIN
	if(c >= UCHAR_MAX)
		return ERR::PATH_CHARACTER_UNSAFE;
#endif

	switch(c)
	{
	case '\\':
	case '/':
	case ':':
	case '"':
	case '?':
	case '*':
	case '<':
	case '>':
	case '|':
	case '^':
		return ERR::PATH_CHARACTER_ILLEGAL;

	default:
		return INFO::OK;
	}
}
