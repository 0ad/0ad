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
 * DLL delay loading and notification
 */

#ifndef INCLUDED_WDLL_DELAY_LOAD
#define INCLUDED_WDLL_DELAY_LOAD

// must be POD because it is used before static ctors run.
struct WdllLoadNotify
{
	const char* dll_name;
	Status (*func)();
	WdllLoadNotify* next;
};

extern void wdll_add_notify(WdllLoadNotify*);

// request that func be called if and when dll_name is ever delay-loaded.
// must be invoked at function scope.
#define WDLL_ADD_NOTIFY(dll_name, func)\
STMT(\
	static WdllLoadNotify UID__ = { dll_name, func };\
	wdll_add_notify(&UID__);\
)

#endif	// #ifndef INCLUDED_WDLL_DELAY_LOAD
