//////////////////////////////////////////////////////////////////////////////
//
//  Core Detours Functionality (detours.h of detours.lib)
//
//  Microsoft Research Detours Package, Version 2.1.
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//

#ifndef _DETOURS_H_
#define _DETOURS_H_

#define DETOURS_X86


#define DETOURS_VERSION     20100   // 2.1.0


#define DETOUR_INSTRUCTION_TARGET_NONE          ((PVOID)0)
#define DETOUR_INSTRUCTION_TARGET_DYNAMIC       ((PVOID)(LONG_PTR)-1)


#define DETOUR_TRAMPOLINE_SIGNATURE             0x21727444  // Dtr!
typedef struct _DETOUR_TRAMPOLINE DETOUR_TRAMPOLINE, *PDETOUR_TRAMPOLINE;


LONG DetourTransactionBegin();
LONG DetourTransactionCommit();
LONG DetourAttach(PVOID *ppPointer, PVOID pDetour);


PVOID WINAPI DetourCopyInstructionEx(PVOID pDst, PVOID pSrc, PVOID *ppTarget, LONG *plExtra);


//////////////////////////////////////////////////////////////////////////////
//
#include <dbghelp.h>

#ifndef DETOUR_TRACE
#if DETOUR_DEBUG
#define DETOUR_TRACE(x) printf x
#define DETOUR_BREAK()  DebugBreak()
#include <stdio.h>
#include <limits.h>
#else
#define DETOUR_TRACE(x)
#define DETOUR_BREAK()
#endif
#endif

#endif // _DETOURS_H_
