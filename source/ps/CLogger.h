#ifndef _ps_CLogger_H
#define _ps_CLogger_H

#include <fstream>
#include <stdio.h>
#include <stdarg.h>
#include <string>
#include <vector>
#include <set>

#define LOG (CLogger::GetInstance()->Log)
#define LOG_ONCE (CLogger::GetInstance()->LogOnce)

enum ELogMethod
{
	NORMAL,
	MESSAGE = NORMAL,
	ERROR,
	WARNING
};

class CLogger
{
public:
	
	//Function used to get instance of CLogger
	static CLogger *GetInstance();
	virtual ~CLogger();

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
	
	CLogger();
	CLogger(const CLogger& init);
	CLogger& operator=(const CLogger& rhs);

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
};

#endif
