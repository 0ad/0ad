/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FUError.h"
#include "FULogFile.h"
#include "FUErrorLog.h"

//
// FUErrorLog
//

FUErrorLog::FUErrorLog(const fchar* logFilename, FUError::Level errorLevel)
:	logFile(NULL), minimumLevel(errorLevel)
{
	FUAssert(errorLevel >= 0 && errorLevel < FUError::LEVEL_COUNT, return);
	logFile = new FULogFile(logFilename);
	counts[0] = counts[1] = counts[2] = 0;

	// Add our callback to the error sinks.
	for (int32 i = FUError::LEVEL_COUNT - 1; i >= minimumLevel; --i)
	{
		FUError::AddErrorCallback((FUError::Level) i, this, &FUErrorLog::OnErrorCallback);
	}
}

FUErrorLog::~FUErrorLog()
{
	// Remove our callback from the error sinks.
	for (int32 i = FUError::LEVEL_COUNT - 1; i >= minimumLevel; --i)
	{
		FUError::RemoveErrorCallback((FUError::Level) i, this, &FUErrorLog::OnErrorCallback);
	}

	SAFE_DELETE(logFile);
}

void FUErrorLog::QueryNewMessages(uint32& debug, uint32& warnings, uint32& errors)
{
	debug = counts[FUError::DEBUG_LEVEL];
	warnings = counts[FUError::WARNING_LEVEL];
	errors = counts[FUError::ERROR_LEVEL];
	counts[0] = counts[1] = counts[2] = 0;
}

void FUErrorLog::OnErrorCallback(FUError::Level level, uint32 errorCode, uint32 argument)
{
	++counts[level];

	FUSStringBuilder newLine(256);
	newLine.append('['); newLine.append(argument); newLine.append("] ");
	if (level == FUError::WARNING_LEVEL) newLine.append("Warning: ");
	else if (level == FUError::ERROR_LEVEL) newLine.append("ERROR: ");
	const char* errorString = FUError::GetErrorString((FUError::Code) errorCode);
	if (errorString != NULL) newLine.append(errorString);
	else
	{
		newLine.append("Unknown error code: ");
		newLine.append(errorCode);
	}

	if (newLine.length() > 0)
	{
#ifdef WIN32
		newLine.append('\r');
#endif // WIN32
		newLine.append('\n');
	}
	logFile->WriteLine(newLine.ToCharPtr());
}
