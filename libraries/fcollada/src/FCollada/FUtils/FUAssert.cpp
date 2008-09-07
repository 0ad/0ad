/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FUAssert.h"
#ifdef __APPLE__
#include <CoreServices/CoreServices.h>
#endif


static FUAssertion::FUAssertCallback* curAssertCallback = NULL;

void FUAssertion::SetAssertionFailedCallback(FUAssertCallback* assertionCallback)
{
	curAssertCallback = assertionCallback;
}

bool FUAssertion::OnAssertionFailed(const char* file, uint32 line)
{
	char message[1024];
	snprintf(message, 1024, "[%s@%u] Assertion failed.\nAbort: Enter debugger.\nRetry: Continue execution.\nIgnore: Do not assert at this line for the duration of the application.", file, (unsigned int) line);
	message[1023] = 0;

	if (curAssertCallback != NULL) return (*curAssertCallback)(message);
#ifdef _DEBUG
	else
	{
#ifdef WIN32
		int32 buttonPressed = MessageBoxA(NULL, message, "Assertion failed.", MB_ABORTRETRYIGNORE | MB_ICONWARNING);
		if (buttonPressed == IDABORT)
		{
			__debugbreak();
		}
		else if (buttonPressed == IDRETRY) {}
		else if (buttonPressed == IDIGNORE) 
		{ 
			return true; 
		}
#elif defined (__APPLE__)
		Debugger();
		//SysBreak();
#else
		// AFAIK This is available on all X86 platforms
		__asm__("int $0x03");
#endif // WIN32
		return false;
	}
#else // _DEBUG
	return false;
#endif // _DEBUG
}
