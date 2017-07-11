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

#ifndef OSX_BUNDLE_H
#define OSX_BUNDLE_H

/**
 * @file
 * C++ interface to Cocoa implementation for getting bundle information
 */

/**
 * Check if app is running in a valid bundle
 *
 * @return true if valid bundle reference was found matching identifier
 *  property "com.wildfiregames.0ad"
 */
bool osx_IsAppBundleValid();

/**
 * Get the system path to the bundle itself
 *
 * @return string containing POSIX-style path in UTF-8 encoding,
 *	else empty string if an error occurred.
 */
std::string osx_GetBundlePath();

/**
 * Get the system path to the bundle's Resources directory
 *
 * @return string containing POSIX-style path in UTF-8 encoding,
 *	else empty string if an error occurred.
 */
std::string osx_GetBundleResourcesPath();

/**
 * Get the system path to the bundle's Frameworks directory
 *
 * @return string containing POSIX-style path in UTF-8 encoding,
 *	else empty string if an error occurred.
 */
std::string osx_GetBundleFrameworksPath();

#endif // OSX_BUNDLE_H
