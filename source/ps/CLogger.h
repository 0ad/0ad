#ifndef _ps_CLogger_H
#define _ps_CLogger_H

#include <fstream>
#include <stdio.h>
#include <stdarg.h>
#include <string>
#include <vector>

#define LOG (CLogger::GetInstance()->Log)

enum ELogMethod
{
	NORMAL,
	MESSAGE=NORMAL,
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
	void WriteMessage(const char *message);
	void WriteError(const char *message);
	void WriteWarning(const char *message);
	
	//Function to log stuff to file
	void Log(ELogMethod method, const char* fmt, ...);
	
	//Function to log stuff to memory buffer
	void QuickLog(const char *fmt, ...);
			
private:
	
	CLogger();
	CLogger(const CLogger& init);
	CLogger& operator=(const CLogger& rhs);

	//the two filestreams
	std::ofstream m_MainLog;
	std::ofstream m_DetailedLog;

	//vars to hold message counts
	int m_NumberOfMessages;
	int m_NumberOfErrors;
	int m_NumberOfWarnings;

	//this holds the start of the memory buffer.
	char *m_MemoryLog;
	//this holds the next available place to write to.
	char *m_CurrentPosition;
};

#endif
