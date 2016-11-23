/* Copyright (c) 2012 Wildfire Games
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

#ifndef OSX_PASTEBOARD_H
#define OSX_PASTEBOARD_H

/**
 * @file
 * C++ interface to Cocoa implementation for pasteboards
 */

/**
 * Get a string from the pasteboard
 *
 * @param[out] out pasteboard string in UTF-8 encoding, if found
 * @return true if string was found on pasteboard and successfully retrieved, false otherwise
 */
bool osx_GetStringFromPasteboard(std::string& out);

/**
 * Store a string on the pasteboard
 *
 * @param[in] string string to store in UTF-8 encoding
 * @return true if string was successfully sent to pasteboard, false on error
 */
bool osx_SendStringToPasteboard(const std::string& string);

#endif // OSX_PASTEBOARD_H
