/* Copyright (C) 2012 Wildfire Games.
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

#ifndef OSX_PATHS_H
#define OSX_PATHS_H

/**
 * @file
 * C++ interface to Cocoa implementation for retrieving standard OS X paths
 */

/**
 * Get the user's Application Support path (typically ~/Library/Application Support)
 *
 * @return string containing POSIX-style path in UTF-8 encoding,
 *	else empty string if an error occurred.
 */
std::string osx_GetAppSupportPath();

/**
 * Get the user's Caches path (typically ~/Library/Caches)
 *
 * @return string containing POSIX-style path in UTF-8 encoding,
 *	else empty string if an error occurred.
 */
std::string osx_GetCachesPath();

#endif // OSX_PATHS_H
