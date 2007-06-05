#include "precompiled.h"

#include "HighResTimer.h"

// TODO: Better accuracy and reliability, if necessary.

#ifdef __WXMSW__

HighResTimer::HighResTimer()
{
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
}

double HighResTimer::GetTime()
{
	LARGE_INTEGER count;
	BOOL ok = QueryPerformanceCounter(&count);
	if (! ok)
	{
		wxLogError(_("QPC failed!"));
		return 0.0;
	}
	return (double)count.QuadPart / (double)m_TickLength.GetValue();
}

#else // not __WXMSW__ :

#include <sys/time.h>

HighResTimer::HighResTimer()
{
}

double HighResTimer::GetTime()
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec+(tv.tv_usec/1000000.0);
}

#endif
