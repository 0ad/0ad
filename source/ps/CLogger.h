#ifndef _ps_CLogger_H
#define _ps_CLogger_H

#include <fstream>
#include <string>
#include <set>

#include "Singleton.h"

#define g_Logger CLogger::GetSingleton()
#define LOG (CLogger::GetSingleton().Log)
#define LOG_ONCE (CLogger::GetSingleton().LogOnce)

enum ELogMethod
{
	NORMAL,
	MESSAGE = NORMAL,
	ERROR,
	WARNING
};

class CLogger : public Singleton<CLogger>
{
public:
	
	CLogger();
	~CLogger();

	//Functions to write different message types
	void WriteMessage(const char *message, int interestedness);
	void WriteError  (const char *message, int interestedness);
	void WriteWarning(const char *message, int interestedness);
	
	//Function to log stuff to file
	void Log(ELogMethod method, const char* category, const char *fmt, ...);
	//Similar to Log, but only outputs each message once no matter how many times it's called
	void LogOnce(ELogMethod method, const char* category, const char *fmt, ...);
	
	//Function to log stuff to memory buffer
	void QuickLog(const char *fmt, ...);

private:

	void LogUsingMethod(ELogMethod method, const char* category, const char* message);

	//the three filestreams
	std::ofstream m_MainLog;
	std::ofstream m_InterestingLog;
	std::ofstream m_MemoryLog;

	//vars to hold message counts
	int m_NumberOfMessages;
	int m_NumberOfErrors;
	int m_NumberOfWarnings;

	// Returns how interesting this category is to the user
	// (0 = no messages, 1(default) = warnings/errors, 2 = all)
	int Interestedness(const char* category);

	//this holds the start of the memory buffer.
	char *m_MemoryLogBuffer;
	//this holds the next available place to write to.
	char *m_CurrentPosition;

	// Used to remember LogOnce messages
	std::set<std::string> m_LoggedOnce;

	// squelch "unable to generate" warnings
	CLogger(const CLogger& rhs);
	const CLogger& operator=(const CLogger& rhs);
};

#endif
