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

#ifndef INCLUDED_VFS_PATH
#define INCLUDED_VFS_PATH

#include "lib/path.h"

/**
 * VFS path of the form "(dir/)*file?"
 *
 * in other words: the root directory is "" and paths are separated by '/'.
 * a trailing slash is allowed for directory names.
 * rationale: it is important to avoid a leading slash because that might be
 * confused with an absolute POSIX path.
 *
 * there is no restriction on path length; when dimensioning character
 * arrays, prefer PATH_MAX.
 **/

typedef Path VfsPath;

typedef std::vector<VfsPath> VfsPaths;

#endif	//	#ifndef INCLUDED_VFS_PATH
