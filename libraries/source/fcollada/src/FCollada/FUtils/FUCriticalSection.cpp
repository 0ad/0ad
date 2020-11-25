/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FUCriticalSection.h"

FUCriticalSection::FUCriticalSection()
{
#ifdef WIN32
	InitializeCriticalSection(&criticalSection);
#endif
}

FUCriticalSection::~FUCriticalSection()
{
#ifdef WIN32
	DeleteCriticalSection(&criticalSection);
#endif
}

void FUCriticalSection::Enter()
{
#ifdef WIN32
	EnterCriticalSection(&criticalSection);
#endif
}

void FUCriticalSection::Leave()
{
#ifdef WIN32
	LeaveCriticalSection(&criticalSection);
#endif
}

