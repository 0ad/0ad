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

bool SelectDLL(int DLL)
{
	const char* name;
	switch (DLL)
	{
	case 0: name = "freetypea.dll"; break;
	case 1: name = "freetypeb.dll"; break;
	default: assert(! "Invalid DLL id!"); return true;
	}

	HINSTANCE hDLL;
	if (dlls[DLL])
		hDLL = dlls[DLL];
	else
		hDLL = dlls[DLL] = LoadLibraryA(name);

	if (! hDLL)
		return true;

	#define FUNC(ret, name, par) if (NULL == (DLL##name = (ret (*) par) GetProcAddress(hDLL, #name)) ) { return true; }
	#include "freetypedll_funcs.h"

	return false;
}

void FreeDLLs()
{
	for (int i=0; i<2; ++i)
		if (dlls[i])
			FreeLibrary(dlls[i]);
}
