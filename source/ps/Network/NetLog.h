#ifndef _Network_NetLog_H
#define _Network_NetLog_H

#include <stdio.h>
#include "ps/ThreadUtil.h"

/*
	CNetLog: an interface for writing to the network trace log. This log is
	intended for spewing. Don't hold back! ;-)
	
	The log should be self-initializing and is only safe to call after main is
	started. Log functions may be called from any thread at any time.
*/
class CNetLog
{
	bool m_Flush;
	bool m_Initialized;
	FILE *m_pFile;
	CMutex m_Mutex;

	/*
		Open the log file for writing.
	*/
	void Open(const char *filename);
	/*
		Flush any pending log entries to disk.
	*/
	void Flush();
	/*
		Open the log file and set up everything (if not already set up)
	*/
	void Initialize();
	
public:
	CNetLog();
	~CNetLog();
	
	/*
		Write a log entry to the network log. Works like printf.
	*/
	void Write(const char *format, ...);
	/*
		Enable/disable flushing after every log line. Flushing starts enabled.
		Setting flush to true will also flush any currently buffered log lines.
	*/
	void SetFlush(bool flush);
};
extern CNetLog g_NetLog;

#define NET_LOG (g_NetLog.Write)

#endif
