/* Copyright (C) 2011 Wildfire Games.
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
 * bring in libcurl header, with compatibility fixes
 */

#ifndef INCLUDED_CURL
#define INCLUDED_CURL

#if OS_WIN

// curl.h wants to include winsock2.h which causes conflicts.
// provide some required definitions from winsock.h, then pretend
// we already included winsock.h

typedef uintptr_t SOCKET;

struct sockaddr
{
	unsigned short sa_family;
	char sa_data[14];
};

struct fd_set;

#define _WINSOCKAPI_	// winsock.h include guard

#endif	// OS_WIN

#include <curl/curl.h>

#endif	// #ifndef INCLUDED_CURL
