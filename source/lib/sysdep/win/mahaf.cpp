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
#include "lib/path_util.h"
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

u8 ReadPort8(u16 port)
{
	const u32 value = ReadPort(port, 1);
	debug_assert(value <= 0xFF);
	return (u8)(value & 0xFF);
}

u16 ReadPort16(u16 port)
{
	const u32 value = ReadPort(port, 2);
	debug_assert(value <= 0xFFFF);
	return (u16)(value & 0xFFFF);
}

u32 ReadPort32(u16 port)
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
	if(!ok)
		WARN_WIN32_ERR;
}

void WritePort8(u16 port, u8 value)
{
	WritePort(port, (u32)value, 1);
}

void WritePort16(u16 port, u16 value)
{
	WritePort(port, (u32)value, 2);
}

void WritePort32(u16 port, u32 value)
{
	WritePort(port, value, 4);
}


void* MapPhysicalMemory(uintptr_t physicalAddress, size_t numBytes)
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


void UnmapPhysicalMemory(void* virtualAddress)
{
	AkenUnmapIn in;
	in.virtualAddress = (DWORD64)virtualAddress;

	DWORD bytesReturned;	// unused but must be passed to DeviceIoControl
	LPOVERLAPPED ovl = 0;	// synchronous
	BOOL ok = DeviceIoControl(hAken, (DWORD)IOCTL_AKEN_UNMAP, &in, sizeof(in), 0, 0u, &bytesReturned, ovl);
	if(!ok)
		WARN_WIN32_ERR;
}


//-----------------------------------------------------------------------------
// driver installation
//-----------------------------------------------------------------------------

static bool Is64BitOs()
{
#if OS_WIN64
	return true;
#else
	// import kernel32!IsWow64Process
	const HMODULE hKernel32Dll = LoadLibrary("kernel32.dll");  
	BOOL (WINAPI *pIsWow64Process)(HANDLE, PBOOL);
	*(void**)&pIsWow64Process = GetProcAddress(hKernel32Dll, "IsWow64Process"); 
	FreeLibrary(hKernel32Dll);

	// function not found => running on 32-bit Windows
	if(!pIsWow64Process)
		return false;

	BOOL isWow64Process = FALSE;
	const BOOL ok = IsWow64Process(GetCurrentProcess(), &isWow64Process);
	WARN_IF_FALSE(ok);

	return (isWow64Process == TRUE);
#endif
}


static SC_HANDLE OpenServiceControlManager()
{
	LPCSTR machineName = 0;	// local
	LPCSTR databaseName = 0;	// default
	SC_HANDLE hSCM = OpenSCManager(machineName, databaseName, SC_MANAGER_ALL_ACCESS);
	// non-admin account => we can't start the driver. note that installing
	// the driver and having it start with Windows would allow access to
	// the service even from least-permission accounts.
	if(!hSCM)
		return 0;

	return hSCM;
}


static void UninstallDriver()
{
	SC_HANDLE hSCM = OpenServiceControlManager();
	if(!hSCM)
		return;
	SC_HANDLE hService = OpenService(hSCM, AKEN_NAME, SERVICE_ALL_ACCESS);
	if(!hService)
		return;

	BOOL ok;
	SERVICE_STATUS serviceStatus;
	ok = ControlService(hService, SERVICE_CONTROL_STOP, &serviceStatus);
	WARN_IF_FALSE(ok);
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

	// create service (note: this just enters the service into SCM's DB;
	// no error is raised if the driver binary doesn't exist etc.)
	SC_HANDLE hService;
	{
create:
		LPCSTR startName = 0;	// LocalSystem
		hService = CreateService(hSCM, AKEN_NAME, AKEN_NAME,
			SERVICE_ALL_ACCESS, SERVICE_KERNEL_DRIVER, SERVICE_DEMAND_START, SERVICE_ERROR_NORMAL,
			driverPathname, 0, 0, 0, startName, 0);
		if(!hService)
		{
			// was already created
			if(GetLastError() == ERROR_SERVICE_EXISTS)
			{
#if 1
				// during development, we want to unload and re-create the
				// service every time to ensure the newest build is used.
				UninstallDriver();
				goto create;
#else
				// in final builds, just use the existing service.
				hService = OpenService(hSCM, AKEN_NAME, SERVICE_ALL_ACCESS);
#endif
			}
			else
				WARN_IF_FALSE(0);	// creating actually failed
		}
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


//-----------------------------------------------------------------------------

static ModuleInitState initState;

bool MahafInit()
{
	if(!ModuleShouldInitialize(&initState))
		return true;

	char driverPathname[PATH_MAX];
	const char* const driverName = Is64BitOs()? "aken64.sys" : "aken.sys";
	(void)path_append(driverPathname, win_exe_dir, driverName);

	StartDriver(driverPathname);

	DWORD shareMode = 0;
	hAken = CreateFile("\\\\.\\Aken", GENERIC_READ, shareMode, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	if(hAken == INVALID_HANDLE_VALUE)
		return false;

	return true;
}


void MahafShutdown()
{
	if(!ModuleShouldShutdown(&initState))
		return;

	CloseHandle(hAken);
	hAken = INVALID_HANDLE_VALUE;

	UninstallDriver();
}
