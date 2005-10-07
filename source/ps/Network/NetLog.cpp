#include "precompiled.h"

#include "NetLog.h"
#include "ThreadUtil.h"
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include "lib/timer.h"

CNetLog g_NetLog;

#define LOG_FORMAT "[%3u.%03u] %s\n"
#define LOG_ARGS_PREFIX (uint)get_time(), ((uint)(get_time()*1000)) % 1000, 
#define LOG_ARGS_SUFFIX 

CNetLog::CNetLog():
	m_Flush(true),
	m_Initialized(false),
	m_pFile(NULL)
{}

CNetLog::~CNetLog()
{
	if (m_Initialized)
	{
		fclose(m_pFile);
	}
}

void CNetLog::Initialize()
{
	CScopeLock scopeLock(m_Mutex);

	if (m_Initialized)
		return;
	
	char filename[256];
	snprintf(filename, sizeof(filename), "../logs/net_log.txt");
	filename[sizeof(filename)-1]=0;
	Open(filename);
	m_Initialized=true;
}

void CNetLog::Open(const char *filename)
{
	m_pFile=fopen(filename, "a");
	if (m_pFile)
	{
		time_t t = time(NULL);
		struct tm* now = localtime(&t);
		
		fprintf(m_pFile, "\n\n***************************************************\n");
		fprintf(m_pFile, "LOG STARTED: %04d-%02d-%02d at %02d:%02d:%02d\n",
			1900+now->tm_year, now->tm_mon+1, now->tm_mday, now->tm_hour,
			now->tm_min, now->tm_sec);
		fprintf(m_pFile, "Timestamps are in seconds since engine startup\n\n");
	}
}

void CNetLog::Flush()
{
	fflush(m_pFile);
}

void CNetLog::Write(const char *format, ...)
{
	Initialize();
	if (!m_pFile) return;

	char buf[512];

	va_list	args;
	va_start(args, format);
	vsnprintf(buf, 512, format, args);
	buf[511]=0;
	va_end(args);
	
	m_Mutex.Lock();
	fprintf(m_pFile, LOG_FORMAT, LOG_ARGS_PREFIX buf LOG_ARGS_SUFFIX);
	if (m_Flush)
		Flush();
	m_Mutex.Unlock();
}

void CNetLog::SetFlush(bool flush)
{
	CScopeLock scopeLock(m_Mutex);
	m_Flush=flush;
	if (m_Flush)
		Flush();
}
