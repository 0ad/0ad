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

/*
 * windows-specific startup code
 */

#include "precompiled.h"
#include "wstartup.h"

#include "winit.h"

/*

Shutdown
--------

our shutdown function must be called after static C++ dtors, since they
may use wpthread and wtime functions. how to accomplish this?

hooking ExitProcess is one way of ensuring we're the very last bit of
code to be called. this has several big problems, though:
- if other exit paths (such as CorExitProcess) are added by MS and
  triggered via external code injected into our process, we miss our cue
  to shut down. one annoying consequence would be that the Aken driver
  remains loaded. however, users also can terminate our process at any
  time, so we need to safely handle lack of shutdown anyway.
- IAT hooking breaks if kernel32 is delay-loaded (who knows what
  people are going to do..)
- Detours-style trampolines are nonportable (the Detours library currently
  only ships with code to handle IA-32) and quite risky, since antivirus
  programs may flag this activity.

having all exit paths call our shutdown and then _cexit would work,
provided we actually find and control all of them (unhandled exceptions
and falling off the end of the last thread are non-obvious examples).
that aside, it'd require changes to calling code, and we're trying to
keep this mess hidden and transparent to users.

the remaining alternative is to use atexit. this approach has the
advantage of being covered by the CRT runtime checks and leak detector,
because those are shut down after the atexit handlers are called.
however, it does require that our shutdown callback to be the very last,
i.e. registered first. fortunately, the init stage can guarantee this.


Init
----

For the same reasons as above, our init really should happen before static
C++ ctors are called.

using WinMain as the entry point and then calling the application's main()
doesn't satisy the above requirement, and is expressly forbidden by ANSI C.
(VC apparently makes use of this and changes its calling convention.
if we call it, everything appears to work but stack traces in release
mode are incorrect, since symbol addresses are off by 4 bytes.)

another alternative is re#defining the app's main function to app_main,
having the OS call our main, and then dispatching to app_main.
however, this leads to trouble when another library (e.g. SDL) wants to
do the same.
moreover, this file is compiled into a static library and used both for
the 0ad executable as well as the separate self-test. this means
we can't enable the main() hook for one and disable it in the other.

requiring users to call us at the beginning of main is brittle in general,
comes after static ctors, and is difficult to achieve in external code
such as the (automatically generated) self-test.

commandeering the entry point, doing init there and then calling
mainCRTStartup would work, but doesn't allow the use of atexit for shutdown
(nor any other non-stateless CRT functions to be called during init).

the way out is to have an init-and-call-atexit function triggered by means
of the CRT init mechanism. we arrange init order such that this happens
before static C++ ctors, thus meeting all of the above requirements.
(note: this is possible because crtexe.c and its .CRT$XI and .CRT$XC
sections holding static initializers and ctors are linked into the
application, not the CRT DLL.)

*/


// reference: see http://www.codeguru.com/cpp/misc/misc/threadsprocesses/article.php/c6945__1

EXTERN_C int wstartup_InitAndRegisterShutdown()
{
	winit_CallInitFunctions();
	atexit(winit_CallShutdownFunctions);
	return 0;
}


// insert our initialization function after _cinit and XIU ("User") block
#if ARCH_AMD64
# define SECTION_ATTRIBUTES read
#else
# define SECTION_ATTRIBUTES read,write
#endif
#pragma section(".CRT$XIV", long,SECTION_ATTRIBUTES)
#undef SECTION_ATTRIBUTES
EXTERN_C __declspec(allocate(".CRT$XIV")) int(*wstartup_pInitAndRegisterShutdown)() = wstartup_InitAndRegisterShutdown;
#pragma comment(linker, "/include:" STRINGIZE(DECORATED_NAME(wstartup_pInitAndRegisterShutdown)))
