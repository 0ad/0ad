#ifndef WIN_INTERNAL_H
#define WIN_INTERNAL_H

#ifndef _WIN32
#error "including win_internal.h without _WIN32 defined"
#endif

#include "lib/types.h"	// intptr_t


// Win32 socket decls aren't portable (e.g. problems with socklen_t)
// => skip winsock.h; lib.h should be used instead
#define _WINSOCKAPI_

#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN


// the public header, win.h, has defined _WINDOWS_ so that
// other code doesn't include <windows.h> when it shouldn't (e.g. zconf.h)
#undef _WINDOWS_

// set version; needed for EnumDisplayDevices
#define _WIN32_WINNT 0x0500



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

typedef struct _PROCESSOR_POWER_INFORMATION
{
	ULONG Number;
	ULONG MaxMhz;
	ULONG CurrentMhz;
	ULONG MhzLimit;
	ULONG MaxIdleState;
	ULONG CurrentIdleState;
} PROCESSOR_POWER_INFORMATION, *PPROCESSOR_POWER_INFORMATION;

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


///////////////////////////////////////////////////////////////////////////////


/* Define _CRTIMP */

#ifndef _CRTIMP
#ifdef  _DLL
#define _CRTIMP __declspec(dllimport)
#else   /* ndef _DLL */
#define _CRTIMP
#endif  /* _DLL */
#endif  /* _CRTIMP */

extern "C" {
_CRTIMP intptr_t _get_osfhandle(int);
_CRTIMP int _open_osfhandle(intptr_t, int);
_CRTIMP int _open(const char* fn, int mode, ...);
_CRTIMP int _close(int);

// can't include malloc.h or crtdbg due to conflicts with mmgr.h
_CRTIMP int _heapchk(void);
#ifndef NDEBUG
_CRTIMP int _CrtSetDbgFlag(int);
#else
#define _CrtSetDbgFlag(f)
#endif
_CRTIMP void  _aligned_free(void *);
_CRTIMP void* _aligned_malloc(size_t, size_t);
_CRTIMP void* _aligned_realloc(void *, size_t, size_t);



/*
* Bit values for _crtDbgFlag flag:
*
* These bitflags control debug heap behavior.
*/

#define _CRTDBG_ALLOC_MEM_DF        0x01  /* Turn on debug allocation */
#define _CRTDBG_DELAY_FREE_MEM_DF   0x02  /* Don't actually free memory */
#define _CRTDBG_CHECK_ALWAYS_DF     0x04  /* Check heap every alloc/dealloc */
#define _CRTDBG_RESERVED_DF         0x08  /* Reserved - do not use */
#define _CRTDBG_CHECK_CRT_DF        0x10  /* Leak check/diff CRT blocks */
#define _CRTDBG_LEAK_CHECK_DF       0x20  /* Leak check at program exit */

/*
We do not check the heap by default at this point because the cost was too high
for some applications. You can still turn this feature on manually.
*/
#define _CRTDBG_CHECK_DEFAULT_DF    0           

#define _CRTDBG_REPORT_FLAG         -1    /* Query bitflag status */




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
// the "segment name" determines when and in what order the functions are
// called: "LIB$W{type}{group}", where {type} is either I for
// (pre-main) initializers, or T for terminators (last of the atexit handlers).
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
