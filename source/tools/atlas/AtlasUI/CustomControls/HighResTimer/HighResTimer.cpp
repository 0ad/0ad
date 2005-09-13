#include "stdafx.h"

#include "HighResTimer.h"

// TODO: Portability and general betterness. (But it's good enough for now.)

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
