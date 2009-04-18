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
 * File        : module_init.cpp
 * Project     : 0 A.D.
 * Description : helpers for module initialization/shutdown.
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#include "precompiled.h"
#include "module_init.h"

#include "lib/sysdep/cpu.h"	// cpu_CAS, cpu_AtomicAdd

// notes:
// - value must be 0 to allow users to just define uninitialized static
//   variables (they don't have access to our MODULE_* symbols)
// - unlike expected in-game operation, the self-tests require repeated
//   sequences of init/shutdown pairs. we therefore allow this in general
//   (resetting back to MODULE_UNINITIALIZED after shutdown) because
//   there's no real disadvantage other than loss of strictness.
static const ModuleInitState MODULE_UNINITIALIZED = 0u;

// (1..N = reference count)

static const ModuleInitState MODULE_ERROR = ~(uintptr_t)1u;


bool ModuleShouldInitialize(volatile ModuleInitState* pInitState)
{
	// currently uninitialized, so give the green light.
	if(cpu_CAS(pInitState, MODULE_UNINITIALIZED, 1))
		return true;

	// increment reference count - unless already in a final state.
retry:
	ModuleInitState latchedInitState = *pInitState;
	if(latchedInitState == MODULE_ERROR)
		return false;
	if(!cpu_CAS(pInitState, latchedInitState, latchedInitState+1))
		goto retry;
	return false;
}


bool ModuleShouldShutdown(volatile ModuleInitState* pInitState)
{
	// decrement reference count - unless already in a final state.
retry:
	ModuleInitState latchedInitState = *pInitState;
	if(latchedInitState == MODULE_UNINITIALIZED || latchedInitState == MODULE_ERROR)
		return false;
	if(!cpu_CAS(pInitState, latchedInitState, latchedInitState-1))
		goto retry;

	// refcount reached zero => allow shutdown.
	if(latchedInitState-1 == MODULE_UNINITIALIZED)
		return true;

	return false;
}


void ModuleSetError(volatile ModuleInitState* pInitState)
{
	*pInitState = MODULE_ERROR;
}


bool ModuleIsError(volatile ModuleInitState* pInitState)
{
	return (*pInitState == MODULE_ERROR);
}
