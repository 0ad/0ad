#ifdef _WIN32

#ifndef __WIN_H__
#define __WIN_H__

// Win32 socket decls aren't portable (e.g. problems with socklen_t)
// => skip winsock.h; posix.h should be used instead

#define _WINSOCKAPI_

#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN

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
#define NOCTLMGR          // Control and Dialog routines
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

// VC6 windows.h may not have defined these
#ifndef INVALID_FILE_ATTRIBUTES
#define INVALID_FILE_ATTRIBUTES ((DWORD)-1)
#endif
#ifndef PROCESSOR_ARCHITECTURE_AMD64
#define PROCESSOR_ARCHITECTURE_AMD64 9
#endif

// HACK: warning-free definition for ICC (value is -1)
#undef INVALID_HANDLE_VALUE
const HANDLE INVALID_HANDLE_VALUE = (HANDLE)(((char*)0) + ~0);

extern "C" {
extern int _get_osfhandle(int);
extern int _open(const char* fn, int mode, ...);
extern int _close(int);
extern int aio_open_winhandle(HANDLE);

#ifndef NO_WINSOCK
#ifdef _MSC_VER
#pragma comment(lib, "wsock32.lib")
#endif
#ifndef _WINSOCK2API_
extern __declspec(dllimport) int __stdcall WSAStartup(WORD, char*);
#endif
#endif
}

#endif
#endif