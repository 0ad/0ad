/* Copyright (C) 2009 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * =========================================================================
 * File        : module_init.h
 * Project     : 0 A.D.
 * Description : helpers for module initialization/shutdown.
 * =========================================================================
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
