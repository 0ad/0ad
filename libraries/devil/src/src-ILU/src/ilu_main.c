//-----------------------------------------------------------------------------
//
// ImageLib Utility Sources
// Copyright (C) 2000-2002 by Denton Woods
// Last modified: 05/20/2001 <--Y2K Compliant! =]
//
// Filename: src-ILU/src/ilu_main.c
//
// Description: Startup functions
//
//-----------------------------------------------------------------------------


#include "ilu_internal.h"
#include "ilu_states.h"
#ifdef _WIN32
	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>
#endif//_WIN32


/* Only needed for MSVC++ unless extended to actually do something =) */
#if defined(_WIN32) && defined(_MSC_VER)

BOOL ILAPIENTRY DllMain(HANDLE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	hModule;  ul_reason_for_call;  lpReserved;
	
	if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
		//iluInit();
	}

	return TRUE;
}

#endif


ILvoid ILAPIENTRY iluInit()
{
	// Used mostly for rotations
	IL_PI = 4 * atan(1);		// precomputed value of pi
	IL_DEGCONV = IL_PI / 180;	// division is slow on some computers
	return;
}


//#ifndef _WIN32_WCE
ILuint ILAPIENTRY iluLoadImage(const ILstring FileName)
{
	ILuint Id;
	ilGenImages(1, &Id);
	if (Id == 0)
		return 0;
	if (!ilLoadImage(FileName)) {
		ilDeleteImages(1, &Id);
		return 0;
	}
	return Id;
}
//#endif//_WIN32_WCE
