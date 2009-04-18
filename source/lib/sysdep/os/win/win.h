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
 * File        : win.h
 * Project     : 0 A.D.
 * Description : bring in <windows.h> with compatibility fixes
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#ifndef INCLUDED_WIN
#define INCLUDED_WIN

#if !OS_WIN
#error "win.h: do not include if not compiling for Windows"
#endif

// Win32 socket declarations aren't portable (e.g. problems with socklen_t)
// => skip winsock.h; posix_sock.h should be used instead.
#define _WINSOCKAPI_

#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN


// other headers may have defined <windows.h>'s include guard to prevent
// external libraries from pulling it in (which would cause conflicts).
#undef _WINDOWS_

// set version; needed for EnumDisplayDevices
#ifndef NTDDI_VERSION
# define NTDDI_VERSION NTDDI_LONGHORN
#endif
#ifndef _WIN32_WINNT
# define _WIN32_WINNT 0x600
#endif

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
//#define NONLS               // All NLS defines and routines
//#define NOMB              // MB_* and MessageBox()
#define NOMEMMGR            // GMEM_*, LMEM_*, GHND, LHND, associated routines
#define NOMETAFILE          // typedef METAFILEPICT
#define NOMINMAX            // Macros min(a,b) and max(a,b)
//#define NOMSG             // typedef MSG and associated routines
#define NOOPENFILE          // OpenFile(), OemToAnsi, AnsiToOem, and OF_*
#define NOSCROLL            // SB_* and scrolling routines
//#define NOSERVICE         // All Service Controller routines, SERVICE_ equates, etc.
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

#include <winreg.h>


//-----------------------------------------------------------------------------
// powrprof.h (missing some parts in VC7)
//-----------------------------------------------------------------------------

// MinGW headers are already correct; only change on VC
#if MSC_VERSION && MSC_VERSION < 1400

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


//-----------------------------------------------------------------------------
// fixes for dbghelp.h 6.4
//-----------------------------------------------------------------------------

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
#ifndef __out_xcount
# define __out_xcount(s)
#endif


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




#if ARCH_IA32
// the official version causes pointer truncation warnings.
# undef InterlockedExchangePointer
# define InterlockedExchangePointer(Target, Value) (PVOID)(uintptr_t)InterlockedExchange((PLONG)(Target), (LONG)(uintptr_t)(Value))
#endif

#endif	// #ifndef INCLUDED_WIN
