#define _WINSOCKAPI_		// sockets already defined by posix.h
#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN
#define _WIN32_WINNT 0x0400	// needed for e.g. mousewheel support
#include <windows.h>
