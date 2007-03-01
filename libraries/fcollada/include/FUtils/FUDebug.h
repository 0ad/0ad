/*
    Copyright (C) 2005-2007 Feeling Software Inc.
    MIT License: http://www.opensource.org/licenses/mit-license.php
*/

/**
	@file FUDebug.h
	This file contains macros useful to write debugging output.
*/

#ifndef _DEBUG_H_
#define _DEBUG_H_

#define FU_LOG_DEBUG	0
#define FU_LOG_WARNING	1
#define FU_LOG_ERROR	2
#define FU_LOG_INDEX	3

/** Outputs a string to the debug monitor.
	@param token The string to output. */
#define DEBUG_OUT(token)	FUDebug::DebugOut(FU_LOG_DEBUG, __FILE__, __LINE__, token);
#define WARNING_OUT(token)	FUDebug::DebugOut(FU_LOG_WARNING, __FILE__, __LINE__, token);
#define ERROR_OUT(token)	FUDebug::DebugOut(FU_LOG_ERROR, __FILE__, __LINE__, token);

/** Outputs a string to the debug monitor.
	@param token The formatted string to output.
	@param arg1 A first argument. */
#define DEBUG_OUT1(token, arg1)		FUDebug::DebugOut(FU_LOG_DEBUG, __FILE__, __LINE__, token, arg1);
#define WARNING_OUT1(token, arg1)	FUDebug::DebugOut(FU_LOG_WARNING, __FILE__, __LINE__, token, arg1);
#define ERROR_OUT1(token, arg1)		FUDebug::DebugOut(FU_LOG_ERROR, __FILE__, __LINE__, token, arg1);

/** Outputs a string to the debug monitor.
	@param token The formatted string to output.
	@param arg1 A first argument.
	@param arg2 A second argument. */
#define DEBUG_OUT2(token, arg1, arg2)		FUDebug::DebugOut(FU_LOG_DEBUG, __FILE__, __LINE__, token, arg1, arg2);
#define WARNING_OUT2(token, arg1, arg2)		FUDebug::DebugOut(FU_LOG_WARNING, __FILE__, __LINE__, token, arg1, arg2);
#define ERROR_OUT2(token, arg1, arg2)		FUDebug::DebugOut(FU_LOG_ERROR, __FILE__, __LINE__, token, arg1, arg2);


class FULogFile;

class FCOLLADA_EXPORT FUDebug
{
private:
	/**	Block access to the constructors and destructors 
		as only static functions of the class will be used. */
	FUDebug();
	virtual ~FUDebug();

	/**	Outputs a string to the debug monitor.
		The formatted message is the first parameter. */
	static void DebugOut(uint8 verbosity, const char* message, ...);
#ifdef UNICODE
	static void DebugOut(uint8 verbosity, const fchar* message, ...); /**< See above. */
#endif // UNICODE

	/**	Outputs a string to the debug monitor.
		The formatted message is the first parameter.
		The second parameter is the variable parmeter list. */
	static void DebugOutV(uint8 verbosity, const char* message, va_list& vars);
#ifdef UNICODE
	static void DebugOutV(uint8 verbosity, const fchar* message, va_list& vars); /**< See above. */
#endif // UNICODE


	/**	Outputs a string to the debug monitor.
		The filename and line number are the first two parameters.
		The formatted message is the third parameter.
		The fourth parameter is the variable parameter list. */
	static void DebugOutV(uint8 verbosity, const char* filename, uint32 line, const char* message, va_list& vars);
#ifdef UNICODE
	static void DebugOutV(uint8 verbosity, const char* filename, uint32 line, const fchar* message, va_list& vars); /**< See above. */
#endif // UNICODE

	
	/**	The log file receiving warning and error messages. */
	static FULogFile* logFile;

	/**	The number of new debug, warning, and error messages since last checked. */
	static uint32 newMessages[FU_LOG_INDEX];

public:

	/**	Outputs a string to the debug monitor.
		The filename and line number are the first two parameters.
		The formatted message is the third parameter. */
	static void DebugOut(uint8 verbosity, const char* filename, uint32 line, const char* message, ...);
#ifdef UNICODE
	static void DebugOut(uint8 verbosity, const char* filename, uint32 line, const fchar* message, ...); /**< See above. */
#endif // UNICODE

	/**	Allows to determine if new messages have been generated
		since the last query. */
	static void QueryNewMessages(uint32& debug, uint32& warnings, uint32& errors);

	/**	This function is called by the viewer to set the log file
		receiving the warnings and errors. */
	static void SetLogFile(FULogFile* _logFile) {logFile = _logFile;}
};
#endif // _DEBUG_H_

