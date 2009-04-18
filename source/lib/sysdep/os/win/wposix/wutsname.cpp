#include "precompiled.h"
#include "wutsname.h"

#include "../wutil.h"

#include "wposix_internal.h"


int uname(struct utsname* un)
{
	static OSVERSIONINFO vi;
	vi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx(&vi);

	// OS implementation name
	strcpy_s(un->sysname, ARRAY_SIZE(un->sysname), wutil_WindowsFamily());

	// release info
	const char* vs = vi.szCSDVersion;
	int sp;
	if(sscanf(vs, "Service Pack %d", &sp) == 1)
		sprintf(un->release, "SP %d", sp);

	// version
	sprintf(un->version, "%s.%lu", wutil_WindowsVersionString(), vi.dwBuildNumber & 0xFFFF);

	// node name
	DWORD buf_size = sizeof(un->nodename);
	DWORD last_err = GetLastError();
	BOOL ok = GetComputerName(un->nodename, &buf_size);
	// GetComputerName sets last error even on success - suppress.
	if(ok)
		SetLastError(last_err);
	else
		debug_assert(0);	// GetComputerName failed

	// hardware type
	static SYSTEM_INFO si;
	GetSystemInfo(&si);
	if(si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64)
		strcpy_s(un->machine, ARRAY_SIZE(un->machine), "AMD64");
	else
		strcpy_s(un->machine, ARRAY_SIZE(un->machine), "IA-32");

	return 0;
}
