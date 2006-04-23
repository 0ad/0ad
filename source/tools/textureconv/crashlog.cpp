#if 0

#pragma comment(lib, "dbghelp.lib")

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <dbghelp.h>
#include <stdlib.h>

#define FILENAME "c:\\texconv_crashlog.dmp"

void DumpMiniDump(HANDLE hFile, PEXCEPTION_POINTERS excpInfo)
{
	MINIDUMP_EXCEPTION_INFORMATION eInfo;
	eInfo.ThreadId = GetCurrentThreadId();
	eInfo.ExceptionPointers = excpInfo;
	eInfo.ClientPointers = FALSE;
	if (!MiniDumpWriteDump(
		GetCurrentProcess(),
		GetCurrentProcessId(),
		hFile,
		MiniDumpNormal,
		excpInfo ? &eInfo : NULL,
		NULL,
		NULL))
	{
	}
}

LONG WINAPI exception_filter(PEXCEPTION_POINTERS ep)
{
	MessageBox(NULL, "Fatal error - generating " FILENAME "...", "Fatal error", MB_OK | MB_ICONERROR);

	HANDLE hFile = CreateFile(FILENAME, GENERIC_WRITE, FILE_SHARE_WRITE, 0, CREATE_ALWAYS, 0, 0);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		MessageBox(NULL, "Failed to generate crash log :-(", "Fatal error", MB_OK | MB_ICONEXCLAMATION);
	}
	else
	{
		DumpMiniDump(hFile, ep);
		CloseHandle(hFile);
	}
	exit(EXIT_FAILURE);
}

struct onInit
{
	onInit()
	{
		SetUnhandledExceptionFilter(exception_filter);
	}
} onInit;


#endif