#include "stdafx.h"

#include "HighResTimer.h"

#ifndef _WIN32
#include <sys/time.h>
#endif

// TODO: Portability and general betterness. (But it's good enough for now.)

HighResTimer::HighResTimer()
{
#ifdef _WIN32
	LARGE_INTEGER freq;
	BOOL ok = QueryPerformanceFrequency(&freq);
	if (! ok)
	{
		wxLogError(_("QPF failed!"));
	}
	else
	{
		m_TickLength = freq.QuadPart;
	}
#endif
}

double HighResTimer::GetTime()
{
#ifdef _WIN32
	LARGE_INTEGER count;
	BOOL ok = QueryPerformanceCounter(&count);
	if (! ok)
	{
		wxLogError(_("QPC failed!"));
		return 0.0;
	}
	return (double)count.QuadPart / (double)m_TickLength.GetValue();
#else
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec+(tv.tv_usec/1000000.0);
#endif
}
