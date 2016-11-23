/* Copyright (c) 2011 Wildfire Games
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

#ifndef INCLUDED_SYSDEP_CLIPBOARD
#define INCLUDED_SYSDEP_CLIPBOARD

// "copy" text into the clipboard. replaces previous contents.
extern Status sys_clipboard_set(const wchar_t* text);

// allow "pasting" from clipboard.
// @return current clipboard text or 0 if not representable as text.
// callers are responsible for passing this pointer to sys_clipboard_free.
extern wchar_t* sys_clipboard_get();

// free memory returned by sys_clipboard_get.
// @param copy is ignored if 0.
extern Status sys_clipboard_free(wchar_t* copy);

#endif	// #ifndef INCLUDED_SYSDEP_CLIPBOARD
