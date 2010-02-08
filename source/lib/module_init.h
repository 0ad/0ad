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
 * helpers for module initialization/shutdown.
 */

#ifndef INCLUDED_MODULE_INIT
#define INCLUDED_MODULE_INIT

/**
 * initialization state of a module: class, source file, or whatever.
 *
 * can be declared as a static variable => no initializer needed,
 * since 0 is the correct initial value.
 *
 * DO NOT change the value directly! (that'd break the carefully thought-out
 * lock-free implementation)
 **/
typedef uintptr_t ModuleInitState;	// uintptr_t required by cpu_CAS

/**
 * @return whether initialization should go forward, i.e. initState is
 * currently MODULE_UNINITIALIZED. increments initState afterwards.
 *
 * (the reason for this function - and tricky part - is thread-safety)
 **/
extern bool ModuleShouldInitialize(volatile ModuleInitState* initState);

/**
 * if module reference count is valid, decrement it.
 * @return whether shutdown should go forward, i.e. this is the last
 * shutdown call.
 **/
extern bool ModuleShouldShutdown(volatile ModuleInitState* initState);

/**
 * indicate the module is unusable, e.g. due to failure during init.
 * all subsequent ModuleShouldInitialize/ModuleShouldShutdown calls
 * for this initState will return false.
 **/
extern void ModuleSetError(volatile ModuleInitState* initState);

/**
 * @return whether the module is in the failure state, i.e. ModuleSetError
 * was previously called on the same initState.
 *
 * this function is provided so that modules can report init failure to
 * the second or later caller.
 **/
extern bool ModuleIsError(volatile ModuleInitState* initState);

#endif	// #ifndef INCLUDED_MODULE_INIT
