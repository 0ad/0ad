/* Copyright (C) 2010 Wildfire Games.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * user-mode interface to Aken driver
 */

#include "precompiled.h"
#include "lib/sysdep/os/win/mahaf.h"

#include "lib/config2.h"
#include "lib/module_init.h"

#include "lib/sysdep/os/win/wutil.h"
#include <winioctl.h>
#include "lib/sysdep/os/win/aken/aken.h"
#include "lib/sysdep/os/win/wversion.h"

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
	const BOOL ok = DeviceIoControl(hAken, (DWORD)IOCTL_AKEN_READ_PORT, &in, sizeof(in), &out, sizeof(out), &bytesReturned, ovl);
	WARN_RETURN_0_IF_FALSE(ok);

	ENSURE(bytesReturned == sizeof(out));
	return out.value;
}

u8 mahaf_ReadPort8(u16 port)
{
	const u32 value = ReadPort(port, 1);
	ENSURE(value <= 0xFF);
	return (u8)(value & 0xFF);
}

u16 mahaf_ReadPort16(u16 port)
{
	const u32 value = ReadPort(port, 2);
	ENSURE(value <= 0xFFFF);
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


bool mahaf_IsPhysicalMappingDangerous()
{
	// pre-XP versions don't prevent re-mapping pages with incompatible
	// attributes, which may lead to disaster due to TLB corruption.
	if(wversion_Number() < WVERSION_XP)
		return true;

	return false;
}


volatile void* mahaf_MapPhysicalMemory(uintptr_t physicalAddress, size_t numBytes)
{
	ENSURE(!mahaf_IsPhysicalMappingDangerous());

	AkenMapIn in;
	in.physicalAddress = (DWORD64)physicalAddress;
	in.numBytes        = (DWORD64)numBytes;
	AkenMapOut out;

	DWORD bytesReturned;
	LPOVERLAPPED ovl = 0;	// synchronous
	const BOOL ok = DeviceIoControl(hAken, (DWORD)IOCTL_AKEN_MAP, &in, sizeof(in), &out, sizeof(out), &bytesReturned, ovl);
	WARN_RETURN_0_IF_FALSE(ok);

	ENSURE(bytesReturned == sizeof(out));
	volatile void* virtualAddress = (volatile void*)(uintptr_t)out.virtualAddress;
	return virtualAddress;
}


void mahaf_UnmapPhysicalMemory(volatile void* virtualAddress)
{
	ENSURE(!mahaf_IsPhysicalMappingDangerous());

	AkenUnmapIn in;
	in.virtualAddress = (DWORD64)virtualAddress;

	DWORD bytesReturned;	// unused but must be passed to DeviceIoControl
	LPOVERLAPPED ovl = 0;	// synchronous
	BOOL ok = DeviceIoControl(hAken, (DWORD)IOCTL_AKEN_UNMAP, &in, sizeof(in), 0, 0u, &bytesReturned, ovl);
	WARN_IF_FALSE(ok);
}


static u64 ReadRegister(DWORD ioctl, u64 reg)
{
	AkenReadRegisterIn in;
	in.reg = reg;
	AkenReadRegisterOut out;

	DWORD bytesReturned;
	LPOVERLAPPED ovl = 0;	// synchronous
	const BOOL ok = DeviceIoControl(hAken, ioctl, &in, sizeof(in), &out, sizeof(out), &bytesReturned, ovl);
	WARN_RETURN_0_IF_FALSE(ok);

	ENSURE(bytesReturned == sizeof(out));
	return out.value;
}

u64 mahaf_ReadModelSpecificRegister(u64 reg)
{
	return ReadRegister((DWORD)IOCTL_AKEN_READ_MSR, reg);
}

u64 mahaf_ReadPerformanceMonitoringCounter(u64 reg)
{
	return ReadRegister((DWORD)IOCTL_AKEN_READ_PMC, reg);
}

void mahaf_WriteModelSpecificRegister(u64 reg, u64 value)
{
	AkenWriteRegisterIn in;
	in.reg = reg;
	in.value = value;

	DWORD bytesReturned;	// unused but must be passed to DeviceIoControl
	LPOVERLAPPED ovl = 0;	// synchronous
	BOOL ok = DeviceIoControl(hAken, (DWORD)IOCTL_AKEN_WRITE_MSR, &in, sizeof(in), 0, 0u, &bytesReturned, ovl);
	WARN_IF_FALSE(ok);
}


//-----------------------------------------------------------------------------
// driver installation
//-----------------------------------------------------------------------------

// @param access need not include SC_MANAGER_CONNECT ("implicitly specified")
static SC_HANDLE OpenServiceControlManager(DWORD access)
{
	LPCWSTR machineName = 0;	// local
	LPCWSTR databaseName = 0;	// default
	SC_HANDLE hSCM = OpenSCManagerW(machineName, databaseName, access);
	if(!hSCM)
	{
		// ensure no other problems arose
		ENSURE(GetLastError() == ERROR_ACCESS_DENIED);

		// administrator privileges are required for SC_MANAGER_CREATE_SERVICE.
		// this is a problem on Vista / Win7, so users will have to use the
		// separate aken_install.bat

		return 0;
	}

	return hSCM;	// success
}


static void UninstallDriver()
{
	SC_HANDLE hSCM = OpenServiceControlManager(SC_MANAGER_ENUMERATE_SERVICE);
	if(!hSCM)
		return;
	SC_HANDLE hService = OpenServiceW(hSCM, AKEN_NAME, SERVICE_STOP|SERVICE_INTERROGATE);
	if(!hService)
		return;

	// stop service
	SERVICE_STATUS serviceStatus;
	if(!ControlService(hService, SERVICE_CONTROL_STOP, &serviceStatus))
	{
		// if the problem wasn't that the service is already stopped,
		// something actually went wrong.
		const DWORD err = GetLastError();
		ENSURE(err == ERROR_SERVICE_NOT_ACTIVE || err == ERROR_SERVICE_CANNOT_ACCEPT_CTRL);
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


#if CONFIG2_MAHAF_ATTEMPT_DRIVER_START

static void StartDriver(const OsPath& driverPathname)
{
	const SC_HANDLE hSCM = OpenServiceControlManager(SC_MANAGER_CREATE_SERVICE);
	if(!hSCM)
	{
		ENSURE(GetLastError() == ERROR_ACCESS_DENIED);
		SetLastError(0);
		return;
	}

	SC_HANDLE hService = OpenServiceW(hSCM, AKEN_NAME, SERVICE_START);

	// during development, we want to ensure the newest build is used, so
	// unload and re-create the service if it's running/installed.
	// as of 2008-03-24 no further changes to Aken are pending, so this is
	// disabled (thus also avoiding trouble when running multiple instances)
#if 0
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
		LPCWSTR startName = 0;	// LocalSystem
		// NB: Windows 7 seems to insist upon backslashes (i.e. external_file_string)
		hService = CreateServiceW(hSCM, AKEN_NAME, AKEN_NAME,
			SERVICE_START, SERVICE_KERNEL_DRIVER, SERVICE_AUTO_START, SERVICE_ERROR_NORMAL,
			OsString(driverPathname).c_str(), 0, 0, 0, startName, 0);
		ENSURE(hService != 0);
	}

	{
		DWORD numArgs = 0;
		BOOL ok = StartService(hService, numArgs, 0);
		if(!ok)
		{
			switch(GetLastError())
			{
			case ERROR_SERVICE_ALREADY_RUNNING:
				// ok, no action needed
				break;
			case ERROR_ACCESS_DENIED:
				// Win7, can't start service; must use aken_install.bat
				break;
			case ERROR_INVALID_IMAGE_HASH:
				// Win7 x86 rejects our code signing certificate; must enable
				// "test signing" mode via aken_install.bat
				break;
			default:
				// unexpected problem
				DEBUG_WARN_ERR(ERR::LOGIC);
				break;
			}
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

static OsPath DriverPathname()
{
	const char* const bits = Is64BitOs()? "64" : "";
#ifdef NDEBUG
	const char* const debug = "";
#else
	const char* const debug = "d";
#endif
	char filename[PATH_MAX];
	sprintf_s(filename, ARRAY_SIZE(filename), "aken%s%s.sys", bits, debug);
	return wutil_ExecutablePath() / filename;
}

#endif // CONFIG2_MAHAF_ATTEMPT_DRIVER_START


//-----------------------------------------------------------------------------

static Status Init()
{
	WinScopedPreserveLastError s;

	if(wutil_HasCommandLineArgument(L"-wNoMahaf"))
		return ERR::NOT_SUPPORTED;	// NOWARN

#if CONFIG2_MAHAF_ATTEMPT_DRIVER_START
	{
		const OsPath driverPathname = DriverPathname();
		StartDriver(driverPathname);
	}
#endif

	{
		const DWORD shareMode = 0;
		hAken = CreateFileW(L"\\\\.\\Aken", GENERIC_READ, shareMode, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
		if(hAken == INVALID_HANDLE_VALUE)
		{
			// GetLastError() is ERROR_FILE_NOT_FOUND if the driver isn't running,
			// but this is also reached when a handle has already been opened
			// (e.g. by a second instance of the same program) - in which case we must
			// indicate failure so that clients won't engage in unsynchronized ring 0 operations.
			SetLastError(0);
			return ERR::INVALID_HANDLE;	// NOWARN (happens often due to security restrictions)
		}
	}

	return INFO::OK;
}

static void Shutdown()
{
	CloseHandle(hAken);
	hAken = INVALID_HANDLE_VALUE;

	UninstallDriver();
}


static ModuleInitState initState;

Status mahaf_Init()
{
	return ModuleInit(&initState, Init);
}

void mahaf_Shutdown()
{
	ModuleShutdown(&initState, Shutdown);
}
