// shared header for Win32-specific code
// Copyright (c) 2002-2005 Jan Wassenberg
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// Contact info:
//   Jan.Wassenberg@stud.uni-karlsruhe.de
//   http://www.stud.uni-karlsruhe.de/~urkt/

#ifndef WIN_INTERNAL_H
#define WIN_INTERNAL_H

#ifndef _WIN32
#error "including win_internal.h without _WIN32 defined"
#endif

#include "lib/types.h"	// intptr_t


// Win32 socket decls aren't portable (e.g. problems with socklen_t)
// => skip winsock.h; wsock.h should be used instead
#define _WINSOCKAPI_

#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN


// the public header, win.h, has defined _WINDOWS_ so that
// other code doesn't include <windows.h> when it shouldn't (e.g. zconf.h)
#undef _WINDOWS_

// set version; needed for EnumDisplayDevices
#define _WIN32_WINNT 0x0501



#define NOGDICAPMASKS     // CC_*, LC_*, PC_*, CP_*, TC_*, RC_
//#define NOVIRTUALKEYCODES // VK_*
//#define NOWINMESSAGES     // WM_*, EM_*, LB_*, CB_*
//#define NOWINSTYLES       // WS_*, CS_*, ES_*, LBS_*, SBS_*, CBS_*
#define NOSYSMETRICS      // SM_*
#define NOMENUS           // MF_*
#define NOICONS           // IDI_*
#define NOKEYSTATES       // MK_*
//#define NOSYSCOMMANDS     // SC_*
#define NORASTEROPS       // Binary and Tertiary raster ops
//#define NOSHOWWINDOW      // SW_*
#define OEMRESOURCE       // OEM Resource values
#define NOATOM            // Atom Manager routines
//#define NOCLIPBOARD       // Clipboard routines
#define NOCOLOR           // Screen colors
//#define NOCTLMGR          // Control and Dialog routines
#define NODRAWTEXT        // DrawText() and DT_*
//#define NOGDI             // All GDI defines and routines
//#define NOKERNEL          // All KERNEL defines and routines
//#define NOUSER            // All USER defines and routines
#define NONLS             // All NLS defines and routines
//#define NOMB              // MB_* and MessageBox()
#define NOMEMMGR          // GMEM_*, LMEM_*, GHND, LHND, associated routines
#define NOMETAFILE        // typedef METAFILEPICT
#define NOMINMAX          // Macros min(a,b) and max(a,b)
//#define NOMSG             // typedef MSG and associated routines
#define NOOPENFILE        // OpenFile(), OemToAnsi, AnsiToOem, and OF_*
#define NOSCROLL          // SB_* and scrolling routines
#define NOSERVICE         // All Service Controller routines, SERVICE_ equates, etc.
//#define NOSOUND           // Sound driver routines
#define NOTEXTMETRIC      // typedef TEXTMETRIC and associated routines
//#define NOWH              // SetWindowsHook and WH_*
#define NOWINOFFSETS      // GWL_*, GCL_*, associated routines
//#define NOCOMM            // COMM driver routines
#define NOKANJI           // Kanji support stuff.
#define NOHELP            // Help engine interface.
#define NOPROFILER        // Profiler interface.
#define NODEFERWINDOWPOS  // DeferWindowPos routines
#define NOMCX             // Modem Configuration Extensions

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

// MinGW headers are already fixed; only change on VC
#ifdef _MSC_VER

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

#endif	// #ifdef _MSC_VER

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
// fixes for dbghelp.h 6.3
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

// missing from dbghelp's list
#define __out_xcount(s)


// not defined by dbghelp; these values are taken from DIA cvconst.h
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

///////////////////////////////////////////////////////////////////////////////


#ifndef _CRTIMP
# ifdef  _DLL
#  define _CRTIMP __declspec(dllimport)
# else
#  define _CRTIMP
# endif
#endif

extern "C" {
extern _CRTIMP intptr_t _get_osfhandle(int);
extern _CRTIMP int _open_osfhandle(intptr_t, int);
extern _CRTIMP int _open(const char* fn, int mode, ...);
extern _CRTIMP int _close(int);

extern _CRTIMP char* _getcwd(char*, size_t);

#ifndef NO_WINSOCK
extern __declspec(dllimport) int __stdcall WSAStartup(unsigned short, void*);
extern __declspec(dllimport) int __stdcall WSACleanup(void);
extern __declspec(dllimport) int __stdcall WSAAsyncSelect(int s, HANDLE hWnd, unsigned int wMsg, long lEvent);
extern __declspec(dllimport) int __stdcall WSAGetLastError(void);
#endif	// #ifndef NO_WINSOCK

extern int WinMainCRTStartup(void);
extern int main(int, char*[]);
}

#define BIT(n) (1ul << (n))
#define FD_READ    BIT(0)
#define FD_WRITE   BIT(1)
#define FD_ACCEPT  BIT(3)
#define FD_CONNECT BIT(4)
#define FD_CLOSE   BIT(5)




//
// locking
//

// critical sections used by win-specific code
enum
{
	ONCE_CS,
	HRT_CS,
	WAIO_CS,
	WIN_CS,
	DBGHELP_CS,

	NUM_CS
};


extern void win_lock(uint idx);
extern void win_unlock(uint idx);



extern void* win_alloc(size_t size);
extern void win_free(void* p);


// thread safe, useable in constructors
#define WIN_ONCE(code) \
{ \
	win_lock(ONCE_CS); \
	static bool ONCE_init_;	/* avoid name conflict */ \
	if(!ONCE_init_) \
	{ \
		ONCE_init_ = true; \
		code; \
	} \
	win_unlock(ONCE_CS); \
}


///////////////////////////////////////////////////////////////////////////////


// init and shutdown mechanism: register a function to be called at
// pre-main init or shutdown.
//
// the "segment name" determines when and in what order the functions are
// called: "LIB$W{type}{group}", where {type} is C for pre-libc init,
// I for pre-main init, or T for terminators (last of the atexit handlers).
// {group} is [B, Y]; groups are called in alphabetical order, but
// call order within the group itself is unspecified.
//
// define the segment via #pragma data_seg(name), register any functions
// to be called via WIN_REGISTER_FUNC, and then restore the previous segment
// with #pragma data_seg() .

#define WIN_REGISTER_FUNC(func) static int func(void); static int(*p##func)(void) = func




#define WIN_SAVE_LAST_ERROR DWORD last_err__ = GetLastError();
#define WIN_RESTORE_LAST_ERROR STMT(if(last_err__ != 0 && GetLastError() == 0) SetLastError(last_err__););


extern char win_sys_dir[MAX_PATH+1];
extern char win_exe_dir[MAX_PATH+1];


#endif	// #ifndef WIN_INTERNAL_H
