/* Copyright (C) 2009 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

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
