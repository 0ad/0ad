#include "stdafx.h"

#include <ft2build.h>
#include FT_FREETYPE_H

#include "freetypedll.h"

#define FUNC(ret, name, par) ret (* DLL##name) par
#include "freetypedll_funcs.h"
#undef FUNC

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

HINSTANCE dlls[2] = { 0, 0 };

static char err[255]; // not exactly thread-safe

char* SelectDLL(int DLL)
{
	const char* name;
	switch (DLL)
	{
	case 0: name = "freetypea.dll"; break;
	case 1: name = "freetypeb.dll"; break;
	default: assert(! "Invalid DLL id!"); return "Invalid DLL ID";
	}

	HINSTANCE hDLL;
	if (dlls[DLL])
		hDLL = dlls[DLL];
	else
		hDLL = dlls[DLL] = LoadLibraryA(name);

	if (! hDLL)
	{
		sprintf(err, "LoadLibrary failed (%d)", GetLastError());
		return err;
	}

	#define FUNC(ret, name, par) if (NULL == (DLL##name = (ret (*) par) GetProcAddress(hDLL, #name)) ) { sprintf(err, "GetProcAddress on %s failed (%d)", #name, GetLastError()); return err; }
	#include "freetypedll_funcs.h"

	return NULL;
}

void FreeDLLs()
{
	for (int i=0; i<2; ++i)
		if (dlls[i])
			FreeLibrary(dlls[i]);
}
