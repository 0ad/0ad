/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FUThread.h"

FUThread::FUThread()
#ifdef WIN32
:	thread(NULL)
#endif
{
}

FUThread::~FUThread()
{
}

void FUThread::YieldCurrentThread()
{
#ifdef WIN32
	SwitchToThread();
#endif
}

void FUThread::SleepCurrentThread(unsigned long milliseconds)
{
#ifdef WIN32
	Sleep(milliseconds);
#endif
}

#ifdef WIN32
FUThread* FUThread::CreateFUThread(LPTHREAD_START_ROUTINE lpStartAddress, void* lpParameter)
#else
FUThread* FUThread::CreateFUThread()
#endif // WIN32
{
#ifdef WIN32
	FUThread* newThread = new FUThread();
	newThread->thread = CreateThread(NULL, 0, lpStartAddress, lpParameter, 0, NULL);
	return newThread;
#else
	return NULL;
#endif
}

void FUThread::ExitFUThread(FUThread* thread)
{	
#ifdef WIN32
	if (thread == NULL) return;

	WaitForSingleObject(thread->thread, INFINITE);
	CloseHandle(thread->thread); // delete the thread once it's finished
	SAFE_DELETE(thread);
#endif
}

