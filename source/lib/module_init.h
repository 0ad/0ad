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
 * initialization state of a module (class, source file, etc.)
 * must be initialized to zero (e.g. by defining as a static variable).
 * DO NOT change the value!
 **/
typedef intptr_t ModuleInitState;	// intptr_t is required by cpu_CAS

/**
 * calls a user-defined init function if initState is zero.
 *
 * @return INFO::SKIPPED if already initialized, a Status if the
 * previous invocation failed, or the value returned by the callback.
 *
 * postcondition: initState is "initialized" if the callback returned
 * INFO::OK, otherwise its Status return value (which prevents
 * shutdown from being called).
 *
 * thread-safe: subsequent callers spin until the callback returns
 * (this prevents using partially-initialized modules)
 *
 * note that callbacks typically reference static data and thus do not
 * require a function argument, but that can later be added if necessary.
 **/
LIB_API Status ModuleInit(volatile ModuleInitState* initState, Status (*init)());

/**
 * calls a user-defined shutdown function if initState is "initialized".
 *
 * @return INFO::OK if shutdown occurred, INFO::SKIPPED if initState was
 * zero (uninitialized), otherwise the Status returned by ModuleInit.
 *
 * postcondition: initState remains set to the Status, or has been
 * reset to zero to allow multiple init/shutdown pairs, e.g. in self-tests.
 *
 * note: there is no provision for reference-counting because that
 * turns out to be problematic (a user might call shutdown immediately
 * after init; if this is the first use of the module, it will
 * be shutdown prematurely, which is at least inefficient and
 * possibly dangerous). instead, shutdown should only be called when
 * cleanup is necessary (e.g. at exit before leak reporting) and
 * it is certain that the module is no longer in use.
 **/
LIB_API Status ModuleShutdown(volatile ModuleInitState* initState, void (*shutdown)());

#endif	// #ifndef INCLUDED_MODULE_INIT
