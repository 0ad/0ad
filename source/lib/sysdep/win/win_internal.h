/**
 * =========================================================================
 * File        : win_internal.h
 * Project     : 0 A.D.
 * Description : shared (private) header for Windows-specific code.
 *
 * @author Jan.Wassenberg@stud.uni-karlsruhe.de
 * =========================================================================
 */

/*
 * Copyright (c) 2002-2005 Jan Wassenberg
 *
 * Redistribution and/or modification are also permitted under the
 * terms of the GNU General Public License as published by the
 * Free Software Foundation (version 2 or later, at your option).
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef WIN_INTERNAL_H
#define WIN_INTERNAL_H

#if !OS_WIN
#error "win_internal.h: do not include if not compiling for Windows"
#endif

#include "lib/lib.h"	// BIT

#if MSC_VERSION >= 1400 // hide "pragma region" from VS2003
#pragma region WindowsHeaderAndFixes
#endif

// Win32 socket declarations aren't portable (e.g. problems with socklen_t)
// => skip winsock.h; posix_sock.h should be used instead.
#define _WINSOCKAPI_

#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN


// the public header, win.h, has defined _WINDOWS_ so that
// other code doesn't include <windows.h> when it shouldn't (e.g. zconf.h)
#undef _WINDOWS_

// set version; needed for EnumDisplayDevices
#define _WIN32_WINNT 0x0501


#define NOGDICAPMASKS       // CC_*, LC_*, PC_*, CP_*, TC_*, RC_
//#define NOVIRTUALKEYCODES // VK_*
//#define NOWINMESSAGES     // WM_*, EM_*, LB_*, CB_*
//#define NOWINSTYLES       // WS_*, CS_*, ES_*, LBS_*, SBS_*, CBS_*
//#define NOSYSMETRICS      // SM_*
#define NOMENUS             // MF_*
//#define NOICONS           // IDI_*
#define NOKEYSTATES         // MK_*
//#define NOSYSCOMMANDS     // SC_*
#define NORASTEROPS         // Binary and Tertiary raster ops
//#define NOSHOWWINDOW      // SW_*
#define OEMRESOURCE         // OEM Resource values
#define NOATOM              // Atom Manager routines
//#define NOCLIPBOARD       // Clipboard routines
//#define NOCOLOR           // Screen colors
//#define NOCTLMGR          // Control and Dialog routines
#define NODRAWTEXT          // DrawText() and DT_*
//#define NOGDI             // All GDI defines and routines
//#define NOKERNEL          // All KERNEL defines and routines
//#define NOUSER            // All USER defines and routines
#define NONLS               // All NLS defines and routines
//#define NOMB              // MB_* and MessageBox()
#define NOMEMMGR            // GMEM_*, LMEM_*, GHND, LHND, associated routines
#define NOMETAFILE          // typedef METAFILEPICT
#define NOMINMAX            // Macros min(a,b) and max(a,b)
//#define NOMSG             // typedef MSG and associated routines
#define NOOPENFILE          // OpenFile(), OemToAnsi, AnsiToOem, and OF_*
#define NOSCROLL            // SB_* and scrolling routines
#define NOSERVICE           // All Service Controller routines, SERVICE_ equates, etc.
//#define NOSOUND           // Sound driver routines
#define NOTEXTMETRIC        // typedef TEXTMETRIC and associated routines
//#define NOWH              // SetWindowsHook and WH_*
//#define NOWINOFFSETS      // GWL_*, GCL_*, associated routines
//#define NOCOMM            // COMM driver routines
#define NOKANJI             // Kanji support stuff.
#define NOHELP              // Help engine interface.
#define NOPROFILER          // Profiler interface.
#define NODEFERWINDOWPOS    // DeferWindowPos routines
#define NOMCX               // Modem Configuration Extensions

#include <windows.h>


///////////////////////////////////////////////////////////////////////////////
//
// fixes for VC6 platform SDK
//
///////////////////////////////////////////////////////////////////////////////

// VC6 windows.h doesn't define these
#ifndef DWORD_PTR
#define DWORD_PTR DWORD
#endif

#ifndef INVALID_FILE_ATTRIBUTES
#define INVALID_FILE_ATTRIBUTES ((DWORD)-1)
#endif

#ifndef PROCESSOR_ARCHITECTURE_AMD64
#define PROCESSOR_ARCHITECTURE_AMD64 9
#endif

#if WINVER < 0x500

// can't test for macro definition -
// actual definitions in winnt.h are typedefs.
typedef unsigned __int64 DWORDLONG;
typedef DWORD ULONG_PTR;

typedef struct _MEMORYSTATUSEX
{ 
	DWORD dwLength;
	DWORD dwMemoryLoad;
	DWORDLONG ullTotalPhys;
	DWORDLONG ullAvailPhys;
	DWORDLONG ullTotalPageFile;
	DWORDLONG ullAvailPageFile;
	DWORDLONG ullTotalVirtual;
	DWORDLONG ullAvailVirtual;
	DWORDLONG ullAvailExtendedVirtual;
} MEMORYSTATUSEX, *LPMEMORYSTATUSEX;

#endif	// #if WINVER < 0x500


///////////////////////////////////////////////////////////////////////////////
//
// powrprof.h (not there at all in VC6, missing some parts in VC7)
//
///////////////////////////////////////////////////////////////////////////////

// MinGW headers are already correct; only change on VC
#if MSC_VERSION

#ifndef NTSTATUS
#define NTSTATUS long
#endif
#ifndef STATUS_SUCCESS
#define STATUS_SUCCESS 0
#endif

#if WINVER < 0x500

typedef enum {
    SystemPowerPolicyAc,
    SystemPowerPolicyDc,
    VerifySystemPolicyAc,
    VerifySystemPolicyDc,
    SystemPowerCapabilities,
    SystemBatteryState,
    SystemPowerStateHandler,
    ProcessorStateHandler,
    SystemPowerPolicyCurrent,
    AdministratorPowerPolicy,
    SystemReserveHiberFile,
    ProcessorInformation,
    SystemPowerInformation,
    ProcessorStateHandler2,
    LastWakeTime,                                   // Compare with KeQueryInterruptTime()
    LastSleepTime,                                  // Compare with KeQueryInterruptTime()
    SystemExecutionState,
    SystemPowerStateNotifyHandler,
    ProcessorPowerPolicyAc,
    ProcessorPowerPolicyDc,
    VerifyProcessorPowerPolicyAc,
    VerifyProcessorPowerPolicyDc,
    ProcessorPowerPolicyCurrent,
    SystemPowerStateLogging,
    SystemPowerLoggingEntry
} POWER_INFORMATION_LEVEL;


typedef struct {
	DWORD       Granularity;
	DWORD       Capacity;
} BATTERY_REPORTING_SCALE, *PBATTERY_REPORTING_SCALE;

typedef enum _SYSTEM_POWER_STATE {
	PowerSystemUnspecified = 0,
	PowerSystemWorking     = 1,
	PowerSystemSleeping1   = 2,
	PowerSystemSleeping2   = 3,
	PowerSystemSleeping3   = 4,
	PowerSystemHibernate   = 5,
	PowerSystemShutdown    = 6,
	PowerSystemMaximum     = 7
} SYSTEM_POWER_STATE, *PSYSTEM_POWER_STATE;

typedef struct {
	// Misc supported system features
	BOOLEAN             PowerButtonPresent;
	BOOLEAN             SleepButtonPresent;
	BOOLEAN             LidPresent;
	BOOLEAN             SystemS1;
	BOOLEAN             SystemS2;
	BOOLEAN             SystemS3;
	BOOLEAN             SystemS4;           // hibernate
	BOOLEAN             SystemS5;           // off
	BOOLEAN             HiberFilePresent;
	BOOLEAN             FullWake;
	BOOLEAN             VideoDimPresent;
	BOOLEAN             ApmPresent;
	BOOLEAN             UpsPresent;

	// Processors
	BOOLEAN             ThermalControl;
	BOOLEAN             ProcessorThrottle;
	BYTE                ProcessorMinThrottle;
	BYTE                ProcessorMaxThrottle;
	BYTE                spare2[4];

	// Disk
	BOOLEAN             DiskSpinDown;
	BYTE                spare3[8];

	// System Battery
	BOOLEAN             SystemBatteriesPresent;
	BOOLEAN             BatteriesAreShortTerm;
	BATTERY_REPORTING_SCALE BatteryScale[3];

	// Wake
	SYSTEM_POWER_STATE  AcOnLineWake;
	SYSTEM_POWER_STATE  SoftLidWake;
	SYSTEM_POWER_STATE  RtcWake;
	SYSTEM_POWER_STATE  MinDeviceWakeState; // note this may change on driver load
	SYSTEM_POWER_STATE  DefaultLowLatencyWake;
} SYSTEM_POWER_CAPABILITIES, *PSYSTEM_POWER_CAPABILITIES;

#endif	// WINVER < 0x500

typedef struct _SYSTEM_POWER_INFORMATION
{
	ULONG MaxIdlenessAllowed;
	ULONG Idleness;
	ULONG TimeRemaining;
	UCHAR CoolingMode;
} SYSTEM_POWER_INFORMATION, *PSYSTEM_POWER_INFORMATION;

// SPI.CoolingMode
#define PO_TZ_INVALID_MODE 0 // The system does not support CPU throttling,
	                         // or there is no thermal zone defined [..]

#endif	// #if MSC_VERSION

// neither VC7.1 nor MinGW define this
typedef struct _PROCESSOR_POWER_INFORMATION
{
	ULONG Number;
	ULONG MaxMhz;
	ULONG CurrentMhz;
	ULONG MhzLimit;
	ULONG MaxIdleState;
	ULONG CurrentIdleState;
} PROCESSOR_POWER_INFORMATION, *PPROCESSOR_POWER_INFORMATION;


///////////////////////////////////////////////////////////////////////////////
//
// fixes for dbghelp.h 6.4
//
///////////////////////////////////////////////////////////////////////////////

// the macros defined "for those without specstrings.h" are incorrect -
// parameter definition is missing.
#ifndef __specstrings
# define __specstrings	// prevent dbghelp from changing these

# define __in
# define __out
# define __inout
# define __in_opt
# define __out_opt
# define __inout_opt
# define __in_ecount(s)
# define __out_ecount(s)
# define __inout_ecount(s)
# define __in_bcount(s)
# define __out_bcount(s)
# define __inout_bcount(s)
# define __deref_opt_out
# define __deref_out

#endif

// (VC2005 defines __specstrings, but doesn't define (or use) __out_xcount,
// so this is not inside the above #ifndef section)
//
// missing from dbghelp's list
# define __out_xcount(s)


//
// not defined by dbghelp; these values are taken from DIA cvconst.h
//

enum BasicType
{
	btNoType    = 0,
	btVoid      = 1,
	btChar      = 2,
	btWChar     = 3,
	btInt       = 6,
	btUInt      = 7,
	btFloat     = 8,
	btBCD       = 9,
	btBool      = 10,
	btLong      = 13,
	btULong     = 14,
	btCurrency  = 25,
	btDate      = 26,
	btVariant   = 27,
	btComplex   = 28,
	btBit       = 29,
	btBSTR      = 30,
	btHresult   = 31
};

enum DataKind
{
	DataIsUnknown,
	DataIsLocal,
	DataIsStaticLocal,
	DataIsParam,
	DataIsObjectPtr,
	DataIsFileStatic,
	DataIsGlobal,
	DataIsMember,
	DataIsStaticMember,
	DataIsConstant
};

#if MSC_VERSION >= 1400 // hide "pragma region" from VS2003
#pragma endregion
#endif

//-----------------------------------------------------------------------------
// locking

// critical sections used by win-specific code
enum
{
	ONCE_CS,
	WTIME_CS,
	WAIO_CS,
	WIN_CS,
	WDBG_CS,

	NUM_CS
};

extern void win_lock(uint idx);
extern void win_unlock(uint idx);

// used in a desperate attempt to avoid deadlock in wdbg_exception_handler.
extern int win_is_locked(uint idx);


//-----------------------------------------------------------------------------
// module init and shutdown

// register functions to be called before libc init, before main,
// or after atexit.
//
// overview:
// participating modules store function pointer(s) to their init and/or
// shutdown function in a specific COFF section. the sections are
// grouped according to the desired notification and the order in which
// functions are to be called (useful if one module depends on another).
// they are then gathered by the linker and arranged in alphabetical order.
// placeholder variables in the sections indicate where the series of
// functions begins and ends for a given notification time.
// at runtime, all of the function pointers between the markers are invoked.
//
// details:
// the section names are of the format ".LIB${type}{group}".
// {type} is C for pre-libc init, I for pre-main init, or
//   T for terminators (last of the atexit handlers).
// {group} is [B, Y]; all functions in a group are called before those of
//   the next (alphabetically) higher group, but order within the group is
//   undefined. this is because the linker sorts sections alphabetically,
//   but doesn't specify the order in which object files are processed.
//   another consequence is that groups A and Z must not be used!
//   (data placed there might end up outside the start/end markers)
//
// example:
// #pragma SECTION_PRE_LIBC(G))
// WIN_REGISTER_FUNC(wtime_init);
// #pragma FORCE_INCLUDE(wtime_init)
// #pragma SECTION_POST_ATEXIT(D))
// WIN_REGISTER_FUNC(wtime_shutdown);
// #pragma FORCE_INCLUDE(wtime_shutdown)
// #pragma SECTION_RESTORE
//
// rationale:
// several methods of module init are possible: (see Large Scale C++ Design)
// - on-demand initialization: each exported function would have to check
//   if init already happened. that would be brittle and hard to verify.
// - singleton: variant of the above, but not applicable to a
//   procedural interface (and quite ugly to boot).
// - registration: NLSO constructors call a central notification function.
//   module dependencies would be quite difficult to express - this would
//   require a graph or separate lists for each priority (clunky).
//   worse, a fatal flaw is that other C++ constructors may depend on the
//   modules we are initializing and already have run. there is no way
//   to influence ctor call order between separate source files, so
//   this is out of the question.
// - linker-based registration: same as above, but the linker takes care
//   of assembling various functions into one sorted table. the list of
//   init functions is available before C++ ctors have run. incidentally,
//   zero runtime overhead is incurred. unfortunately, this approach is
//   MSVC-specific. however, the MS CRT uses a similar method for its
//   init, so this is expected to remain supported.

// macros to simplify usage.
// notes:
// - #pragma cannot be packaged in macros due to expansion rules.
// - __declspec(allocate) would be tempting, since that could be
//   wrapped in WIN_REGISTER_FUNC. unfortunately it inexplicably cannot
//   cope with split string literals (e.g. "ab" "c"). that disqualifies
//   it, since we want to hide the section name behind a macro, which
//   would require the abovementioned merging.

// note: the purpose of pre-libc init (with the resulting requirement that
// no CRT functions be used during init!) is to allow the use of the
// initialized module in static ctors.
#define SECTION_PRE_LIBC(group)    data_seg(".LIB$C" #group)
#define SECTION_PRE_MAIN(group)    data_seg(".LIB$I" #group)
#define SECTION_POST_ATEXIT(group) data_seg(".LIB$T" #group)
#define SECTION_RESTORE data_seg()
// use to make sure the link-stage optimizer doesn't discard the
// function pointers (happens on VC8)
#define FORCE_INCLUDE(id) comment(linker, "/include:_p"#id)

#define WIN_REGISTER_FUNC(func)\
	static LibError func(void);\
	extern "C" LibError (*p##func)(void) = func


//-----------------------------------------------------------------------------
// misc

extern void* win_alloc(size_t size);
extern void win_free(void* p);


// thread safe, usable in constructors
#define WIN_ONCE(code)\
{\
	win_lock(ONCE_CS);\
	static bool ONCE_init_;	/* avoid name conflict */\
	if(!ONCE_init_)\
	{\
		ONCE_init_ = true;\
		code;\
	}\
	win_unlock(ONCE_CS);\
}

#define WIN_SAVE_LAST_ERROR DWORD last_err__ = GetLastError();
#define WIN_RESTORE_LAST_ERROR STMT(if(last_err__ != 0 && GetLastError() == 0) SetLastError(last_err__););


// return the LibError equivalent of GetLastError(), or ERR::FAIL if
// there's no equal.
// you should SetLastError(0) before calling whatever will set ret
// to make sure we do not return any stale errors.
extern LibError LibError_from_win32(DWORD ret, bool warn_if_failed = true);


extern char win_sys_dir[MAX_PATH+1];
extern char win_exe_dir[MAX_PATH+1];


// this isn't nice (ideally we would avoid coupling win.cpp and wdbg.cpp), but
// necessary; see rationale at function definition.
extern LONG WINAPI wdbg_exception_filter(EXCEPTION_POINTERS* ep);




#ifdef USE_WINMAIN
extern "C" int WinMainCRTStartup(void);
#else
extern "C" int mainCRTStartup(void);
#endif


#define FD_READ    BIT(0)
#define FD_WRITE   BIT(1)
#define FD_ACCEPT  BIT(3)
#define FD_CONNECT BIT(4)
#define FD_CLOSE   BIT(5)


#endif	// #ifndef WIN_INTERNAL_H
