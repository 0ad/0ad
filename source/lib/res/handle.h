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

/*
 * forward declaration of Handle (reduces dependencies)
 */

#ifndef INCLUDED_HANDLE
#define INCLUDED_HANDLE

/**
 * `handle' representing a reference to a resource (sound, texture, etc.)
 *
 * 0 is the (silently ignored) invalid handle value; < 0 is an error code.
 *
 * this is 64 bits because we want tags to remain unique. (tags are a
 * counter that disambiguate several subsequent uses of the same
 * resource array slot). 32-bit handles aren't enough because the index
 * field requires at least 12 bits, thus leaving only about 512K possible
 * tag values.
 **/
typedef i64 Handle;

#endif	// #ifndef INCLUDED_HANDLE
