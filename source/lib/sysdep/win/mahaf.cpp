/**
 * =========================================================================
 * File        : mahaf.cpp
 * Project     : 0 A.D.
 * Description : user-mode interface to Aken driver
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#include "precompiled.h"

#include "win.h"
#include <winioctl.h>
#include "aken/aken.h"
#include "wutil.h"
#include "lib/module_init.h"


static HANDLE hAken = INVALID_HANDLE_VALUE;	// handle to Aken driver

//-----------------------------------------------------------------------------
// ioctl wrappers
//-----------------------------------------------------------------------------

static u32 ReadPort(u16 port, u8 numBytes)
{
	AkenReadPortIn in;
	in.port = (USHORT)port;
	in.numBytes = (UCHAR)numBytes;
	AkenReadPortOut out;

	DWORD bytesReturned;
	LPOVERLAPPED ovl = 0;	// synchronous
	BOOL ok = DeviceIoControl(hAken, (DWORD)IOCTL_AKEN_READ_PORT, &in, sizeof(in), &out, sizeof(out), &bytesReturned, ovl);
	if(!ok)
	{
		WARN_WIN32_ERR;
		return 0;
	}

	debug_assert(bytesReturned == sizeof(out));
	const u32 value = out.value;
	return value;
}

u8 mahaf_ReadPort8(u16 port)
{
	const u32 value = ReadPort(port, 1);
	debug_assert(value <= 0xFF);
	return (u8)(value & 0xFF);
}

u16 mahaf_ReadPort16(u16 port)
{
	const u32 value = ReadPort(port, 2);
	debug_assert(value <= 0xFFFF);
	return (u16)(value & 0xFFFF);
}

u32 mahaf_ReadPort32(u16 port)
{
	const u32 value = ReadPort(port, 4);
	return value;
}


static void WritePort(u16 port, u32 value, u8 numBytes)
{
	AkenWritePortIn in;
	in.value = (DWORD32)value;
	in.port  = (USHORT)port;
	in.numBytes = (UCHAR)numBytes;

	DWORD bytesReturned;	// unused but must be passed to DeviceIoControl
	LPOVERLAPPED ovl = 0;	// synchronous
	BOOL ok = DeviceIoControl(hAken, (DWORD)IOCTL_AKEN_WRITE_PORT, &in, sizeof(in), 0, 0u, &bytesReturned, ovl);
	WARN_IF_FALSE(ok);
}

void mahaf_WritePort8(u16 port, u8 value)
{
	WritePort(port, (u32)value, 1);
}

void mahaf_WritePort16(u16 port, u16 value)
{
	WritePort(port, (u32)value, 2);
}

void mahaf_WritePort32(u16 port, u32 value)
{
	WritePort(port, value, 4);
}


void* mahaf_MapPhysicalMemory(uintptr_t physicalAddress, size_t numBytes)
{
	AkenMapIn in;
	in.physicalAddress = (DWORD64)physicalAddress;
	in.numBytes        = (DWORD64)numBytes;
	AkenMapOut out;

	DWORD bytesReturned;
	LPOVERLAPPED ovl = 0;	// synchronous
	BOOL ok = DeviceIoControl(hAken, (DWORD)IOCTL_AKEN_MAP, &in, sizeof(in), &out, sizeof(out), &bytesReturned, ovl);
	if(!ok)
	{
		WARN_WIN32_ERR;
		return 0;
	}

	debug_assert(bytesReturned == sizeof(out));
	void* virtualAddress = (void*)out.virtualAddress;
	return virtualAddress;
}


void mahaf_UnmapPhysicalMemory(void* virtualAddress)
{
	AkenUnmapIn in;
	in.virtualAddress = (DWORD64)virtualAddress;

	DWORD bytesReturned;	// unused but must be passed to DeviceIoControl
	LPOVERLAPPED ovl = 0;	// synchronous
	BOOL ok = DeviceIoControl(hAken, (DWORD)IOCTL_AKEN_UNMAP, &in, sizeof(in), 0, 0u, &bytesReturned, ovl);
	WARN_IF_FALSE(ok);
}


bool mahaf_CopyPhysicalMemory(uintptr_t physicalAddress, size_t numBytes, void* buffer)
{
	AkenCopyPhysicalIn in;
	in.physicalAddress = (DWORD64)physicalAddress;
	in.numBytes        = (DWORD64)numBytes;
	in.userAddress     = (DWORD64)buffer;

	DWORD bytesReturned;
	LPOVERLAPPED ovl = 0;	// synchronous
	BOOL ok = DeviceIoControl(hAken, (DWORD)IOCTL_AKEN_COPY_PHYSICAL, &in, sizeof(in), 0, 0, &bytesReturned, ovl);
	if(!ok)
	{
		WARN_WIN32_ERR;
		return false;
	}

	return true;
}


//-----------------------------------------------------------------------------
// driver installation
//-----------------------------------------------------------------------------

static SC_HANDLE OpenServiceControlManager()
{
	LPCSTR machineName = 0;	// local
	LPCSTR databaseName = 0;	// default
	SC_HANDLE hSCM = OpenSCManager(machineName, databaseName, SC_MANAGER_ALL_ACCESS);
	if(!hSCM)
	{
		// administrator privileges are required. note: installing the
		// service and having it start automatically would allow
		// Least-Permission accounts to use it, but is too invasive and
		// thus out of the question.

		// make sure the error is as expected, otherwise something is afoot.
		if(GetLastError() != ERROR_ACCESS_DENIED)
			debug_assert(0);

		return 0;
	}

	return hSCM;	// success
}


static void UninstallDriver()
{
	SC_HANDLE hSCM = OpenServiceControlManager();
	if(!hSCM)
		return;
	SC_HANDLE hService = OpenService(hSCM, AKEN_NAME, SERVICE_ALL_ACCESS);
	if(!hService)
		return;

	// stop service
	SERVICE_STATUS serviceStatus;
	if(!ControlService(hService, SERVICE_CONTROL_STOP, &serviceStatus))
	{
		// if the problem wasn't that the service is already stopped,
		// something actually went wrong.
		const DWORD err = GetLastError();
		if(err != ERROR_SERVICE_NOT_ACTIVE && err != ERROR_SERVICE_CANNOT_ACCEPT_CTRL)
			debug_assert(0);
	}

	// delete service
	BOOL ok;
	ok = DeleteService(hService);
	WARN_IF_FALSE(ok);
	ok = CloseServiceHandle(hService);
	WARN_IF_FALSE(ok);

	ok = CloseServiceHandle(hSCM);
	WARN_IF_FALSE(ok);
}


static void StartDriver(const char* driverPathname)
{
	const SC_HANDLE hSCM = OpenServiceControlManager();
	if(!hSCM)
		return;

	SC_HANDLE hService = OpenService(hSCM, AKEN_NAME, SERVICE_ALL_ACCESS);

#if 1
	// during development, we want to ensure the newest build is used, so
	// unload and re-create the service if it's running/installed.
	if(hService)
	{
		BOOL ok = CloseServiceHandle(hService);
		WARN_IF_FALSE(ok);
		hService = 0;
		UninstallDriver();
	}
#endif	

	// create service (note: this just enters the service into SCM's DB;
	// no error is raised if the driver binary doesn't exist etc.)
	if(!hService)
	{
		LPCSTR startName = 0;	// LocalSystem
		hService = CreateService(hSCM, AKEN_NAME, AKEN_NAME,
			SERVICE_ALL_ACCESS, SERVICE_KERNEL_DRIVER, SERVICE_DEMAND_START, SERVICE_ERROR_NORMAL,
			driverPathname, 0, 0, 0, startName, 0);
		debug_assert(hService != 0);
	}

	// start service
	{
		DWORD numArgs = 0;
		BOOL ok = StartService(hService, numArgs, 0);
		if(!ok)
		{
			// if it wasn't already running, starting failed
			if(GetLastError() != ERROR_SERVICE_ALREADY_RUNNING)
				WARN_IF_FALSE(0);
		}
	}

	CloseServiceHandle(hService);
	CloseServiceHandle(hSCM);
}


static bool Is64BitOs()
{
#if OS_WIN64
	return true;
#else
	return wutil_IsWow64();
#endif
}

static void GetDriverPathname(char* driverPathname, size_t maxChars)
{
	const char* const bits = Is64BitOs()? "64" : "";
#ifdef NDEBUG
	const char* const debug = "";
#else
	const char* const debug = "d";
#endif
	sprintf_s(driverPathname, maxChars, "%s\\aken%s%s.sys", win_exe_dir, bits, debug);
}


//-----------------------------------------------------------------------------

static ModuleInitState initState;

bool mahaf_Init()
{
	if(!ModuleShouldInitialize(&initState))
		return true;

	char driverPathname[PATH_MAX];
	GetDriverPathname(driverPathname, ARRAY_SIZE(driverPathname));

	StartDriver(driverPathname);

	DWORD shareMode = 0;
	hAken = CreateFile("\\\\.\\Aken", GENERIC_READ, shareMode, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	if(hAken == INVALID_HANDLE_VALUE)
	{
		ModuleSetError(&initState);
		return false;
	}

	return true;
}


void mahaf_Shutdown()
{
	if(!ModuleShouldShutdown(&initState))
		return;

	CloseHandle(hAken);
	hAken = INVALID_HANDLE_VALUE;

	UninstallDriver();
}
