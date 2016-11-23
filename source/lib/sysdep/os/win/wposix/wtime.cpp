/* Copyright (c) 2010 Wildfire Games
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * emulate POSIX time functionality on Windows.
 */

// note: clock_gettime et al. have been removed. callers should use the
// WHRT directly, rather than needlessly translating s -> ns -> s,
// which costs time and accuracy.

#include "precompiled.h"
#include "lib/sysdep/os/win/wposix/wtime.h"

#include "lib/sysdep/os/win/wposix/wposix_internal.h"
#include "lib/sysdep/os/win/whrt/whrt.h"

WINIT_REGISTER_MAIN_INIT(wtime_Init);	// whrt -> wtime

// NT system time and FILETIME are hectonanoseconds since Jan. 1, 1601 UTC.
// SYSTEMTIME is a struct containing month, year, etc.

static const long _1e3 = 1000;
static const long _1e6 = 1000000;
static const long _1e7 = 10000000;
static const i64  _1e9 = 1000000000;


//
// FILETIME -> time_t routines; used by wposix filetime_to_time_t wrapper.
//

// hectonanoseconds between Windows and POSIX epoch
static const u64 posix_epoch_hns = 0x019DB1DED53E8000;

// this function avoids the pitfall of casting FILETIME* to u64*,
// which is not safe due to differing alignment guarantees!
// on some platforms, that would result in an exception.
static u64 u64_from_FILETIME(const FILETIME* ft)
{
	return u64_from_u32(ft->dwHighDateTime, ft->dwLowDateTime);
}


// convert UTC FILETIME to seconds-since-1970 UTC:
// we just have to subtract POSIX epoch and scale down to units of seconds.
//
// used by wfilesystem.
//
// note: RtlTimeToSecondsSince1970 isn't officially documented,
// so don't use that.
time_t wtime_utc_filetime_to_time_t(FILETIME* ft)
{
	u64 hns = u64_from_FILETIME(ft);
	u64 s = (hns - posix_epoch_hns) / _1e7;
	return (time_t)(s & 0xFFFFFFFF);
}


//-----------------------------------------------------------------------------

// system clock at startup [nanoseconds since POSIX epoch]
// note: the HRT starts at 0; any increase by the time we get here
// just makes our notion of the start time more accurate)
static i64 stInitial_ns;

static void LatchInitialSystemTime()
{
	FILETIME ft;
	GetSystemTimeAsFileTime(&ft);
	const u64 hns = u64_from_FILETIME(&ft);
	stInitial_ns = (hns - posix_epoch_hns) * 100;
}

// return nanoseconds since POSIX epoch.
// algorithm: add current HRT value to the startup system time
static i64 CurrentSystemTime_ns()
{
	const i64 ns = stInitial_ns + i64(whrt_Time() * _1e9);
	return ns;
}

static timespec TimespecFromNs(i64 ns)
{
	timespec ts;
	ts.tv_sec = (time_t)((ns / _1e9) & 0xFFFFFFFF);
	ts.tv_nsec = (long)(ns % _1e9);
	return ts;
}

static size_t MsFromTimespec(const timespec& ts)
{
	i64 ms = ts.tv_sec;	// avoid overflow
	ms *= _1e3;
	ms += ts.tv_nsec / _1e6;
	return size_t(ms);
}


//-----------------------------------------------------------------------------

int clock_gettime(clockid_t clock, struct timespec* ts)
{
	ENSURE(clock == CLOCK_REALTIME || clock == CLOCK_MONOTONIC);

	const i64 ns = CurrentSystemTime_ns();
	*ts = TimespecFromNs(ns);
	return 0;
}


int clock_getres(clockid_t clock, struct timespec* ts)
{
	ENSURE(clock == CLOCK_REALTIME || clock == CLOCK_MONOTONIC);

	const double resolution = whrt_Resolution();
	const i64 ns = i64(resolution * 1e9);
	*ts = TimespecFromNs(ns);
	return 0;
}


int nanosleep(const struct timespec* rqtp, struct timespec* /* rmtp */)
{
	const DWORD ms = (DWORD)MsFromTimespec(*rqtp);
	if(ms)
		Sleep(ms);
	return 0;
}


unsigned sleep(unsigned sec)
{
	// warn if overflow would result (it would be insane to ask for
	// such lengthy sleep timeouts, but still)
	ENSURE(sec < std::numeric_limits<size_t>::max()/1000);

	const DWORD ms = sec * 1000;
	if(ms)
		Sleep(ms);
	return sec;
}


int usleep(useconds_t us)
{
	ENSURE(us < _1e6);

	const DWORD ms = us/1000;
	if(ms)
		Sleep(ms);
	return 0;
}


//-----------------------------------------------------------------------------
// strptime from NetBSD

/*
* Copyright (c) 1999 Kungliga Tekniska Hogskolan
* (Royal Institute of Technology, Stockholm, Sweden).
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions
* are met:
*
* 1. Redistributions of source code must retain the above copyright
*    notice, this list of conditions and the following disclaimer.
*
* 2. Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in the
*    documentation and/or other materials provided with the distribution.
*
* 3. Neither the name of KTH nor the names of its contributors may be
*    used to endorse or promote products derived from this software without
*    specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY KTH AND ITS CONTRIBUTORS ``AS IS'' AND ANY
* EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
* PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL KTH OR ITS CONTRIBUTORS BE
* LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
* BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
* WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
* OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
* ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. */

static const char *abb_weekdays[] = {
	"Sun",
	"Mon",
	"Tue",
	"Wed",
	"Thu",
	"Fri",
	"Sat",
	NULL
};

static const char *full_weekdays[] = {
	"Sunday",
	"Monday",
	"Tuesday",
	"Wednesday",
	"Thursday",
	"Friday",
	"Saturday",
	NULL
};

static const char *abb_month[] = {
	"Jan",
	"Feb",
	"Mar",
	"Apr",
	"May",
	"Jun",
	"Jul",
	"Aug",
	"Sep",
	"Oct",
	"Nov",
	"Dec",
	NULL
};

static const char *full_month[] = {
	"January",
	"February",
	"Mars",
	"April",
	"May",
	"June",
	"July",
	"August",
	"September",
	"October",
	"November",
	"December",
	NULL,
};

static const char *ampm[] = {
	"am",
	"pm",
	NULL
};

/*
* Try to match `*buf' to one of the strings in `strs'.  Return the
* index of the matching string (or -1 if none).  Also advance buf.
*/

static int
match_string (const char **buf, const char **strs)
{
	int i = 0;

	for (i = 0; strs[i] != NULL; ++i) {
		size_t len = strlen (strs[i]);

		if (strncasecmp (*buf, strs[i], len) == 0) {
			*buf += len;
			return i;
		}
	}
	return -1;
}

/*
* tm_year is relative this year */

const int tm_year_base = 1900;

/*
* Return TRUE iff `year' was a leap year.
*/

static int
is_leap_year (int year)
{
	return (year % 4) == 0 && ((year % 100) != 0 || (year % 400) == 0);
}

/*
* Return the weekday [0,6] (0 = Sunday) of the first day of `year'
*/

static int
first_day (int year)
{
	int ret = 4;

	for (; year > 1970; --year)
		ret = (ret + 365 + is_leap_year (year) ? 1 : 0) % 7;
	return ret;
}

/*
* Set `timeptr' given `wnum' (week number [0, 53])
*/

static void
set_week_number_sun (struct tm *timeptr, int wnum)
{
	int fday = first_day (timeptr->tm_year + tm_year_base);

	timeptr->tm_yday = wnum * 7 + timeptr->tm_wday - fday;
	if (timeptr->tm_yday < 0) {
		timeptr->tm_wday = fday;
		timeptr->tm_yday = 0;
	}
}

/*
* Set `timeptr' given `wnum' (week number [0, 53])
*/

static void
set_week_number_mon (struct tm *timeptr, int wnum)
{
	int fday = (first_day (timeptr->tm_year + tm_year_base) + 6) % 7;

	timeptr->tm_yday = wnum * 7 + (timeptr->tm_wday + 6) % 7 - fday;
	if (timeptr->tm_yday < 0) {
		timeptr->tm_wday = (fday + 1) % 7;
		timeptr->tm_yday = 0;
	}
}

/*
* Set `timeptr' given `wnum' (week number [0, 53])
*/

static void
set_week_number_mon4 (struct tm *timeptr, int wnum)
{
	int fday = (first_day (timeptr->tm_year + tm_year_base) + 6) % 7;
	int offset = 0;

	if (fday < 4)
		offset += 7;

	timeptr->tm_yday = offset + (wnum - 1) * 7 + timeptr->tm_wday - fday;
	if (timeptr->tm_yday < 0) {
		timeptr->tm_wday = fday;
		timeptr->tm_yday = 0;
	}
}

/*
*
*/

char *
strptime (const char *buf, const char *format, struct tm *timeptr)
{
	char c;

	for (; (c = *format) != '\0'; ++format) {
		char *s;
		int ret;

		if (isspace (c)) {
			while (isspace (*buf))
				++buf;
		} else if (c == '%' && format[1] != '\0') {
			c = *++format;
			if (c == 'E' || c == 'O')
				c = *++format;
			switch (c) {
		case 'A' :
			ret = match_string (&buf, full_weekdays);
			if (ret < 0)
				return NULL;
			timeptr->tm_wday = ret;
			break;
		case 'a' :
			ret = match_string (&buf, abb_weekdays);
			if (ret < 0)
				return NULL;
			timeptr->tm_wday = ret;
			break;
		case 'B' :
			ret = match_string (&buf, full_month);
			if (ret < 0)
				return NULL;
			timeptr->tm_mon = ret;
			break;
		case 'b' :
		case 'h' :
			ret = match_string (&buf, abb_month);
			if (ret < 0)
				return NULL;
			timeptr->tm_mon = ret;
			break;
		case 'C' :
			ret = strtol (buf, &s, 10);
			if (s == buf)
				return NULL;
			timeptr->tm_year = (ret * 100) - tm_year_base;
			buf = s;
			break;
		case 'c' :
			abort ();
		case 'D' :		/* %m/%d/%y */
			s = strptime (buf, "%m/%d/%y", timeptr);
			if (s == NULL)
				return NULL;
			buf = s;
			break;
		case 'd' :
		case 'e' :
			ret = strtol (buf, &s, 10);
			if (s == buf)
				return NULL;
			timeptr->tm_mday = ret;
			buf = s;
			break;
		case 'H' :
		case 'k' :
			ret = strtol (buf, &s, 10);
			if (s == buf)
				return NULL;
			timeptr->tm_hour = ret;
			buf = s;
			break;
		case 'I' :
		case 'l' :
			ret = strtol (buf, &s, 10);
			if (s == buf)
				return NULL;
			if (ret == 12)
				timeptr->tm_hour = 0;
			else
				timeptr->tm_hour = ret;
			buf = s;
			break;
		case 'j' :
			ret = strtol (buf, &s, 10);
			if (s == buf)
				return NULL;
			timeptr->tm_yday = ret - 1;
			buf = s;
			break;
		case 'm' :
			ret = strtol (buf, &s, 10);
			if (s == buf)
				return NULL;
			timeptr->tm_mon = ret - 1;
			buf = s;
			break;
		case 'M' :
			ret = strtol (buf, &s, 10);
			if (s == buf)
				return NULL;
			timeptr->tm_min = ret;
			buf = s;
			break;
		case 'n' :
			if (*buf == '\n')
				++buf;
			else
				return NULL;
			break;
		case 'p' :
			ret = match_string (&buf, ampm);
			if (ret < 0)
				return NULL;
			if (timeptr->tm_hour == 0) {
				if (ret == 1)
					timeptr->tm_hour = 12;
			} else
				timeptr->tm_hour += 12;
			break;
		case 'r' :		/* %I:%M:%S %p */
			s = strptime (buf, "%I:%M:%S %p", timeptr);
			if (s == NULL)
				return NULL;
			buf = s;
			break;
		case 'R' :		/* %H:%M */
			s = strptime (buf, "%H:%M", timeptr);
			if (s == NULL)
				return NULL;
			buf = s;
			break;
		case 'S' :
			ret = strtol (buf, &s, 10);
			if (s == buf)
				return NULL;
			timeptr->tm_sec = ret;
			buf = s;
			break;
		case 't' :
			if (*buf == '\t')
				++buf;
			else
				return NULL;
			break;
		case 'T' :		/* %H:%M:%S */
		case 'X' :
			s = strptime (buf, "%H:%M:%S", timeptr);
			if (s == NULL)
				return NULL;
			buf = s;
			break;
		case 'u' :
			ret = strtol (buf, &s, 10);
			if (s == buf)
				return NULL;
			timeptr->tm_wday = ret - 1;
			buf = s;
			break;
		case 'w' :
			ret = strtol (buf, &s, 10);
			if (s == buf)
				return NULL;
			timeptr->tm_wday = ret;
			buf = s;
			break;
		case 'U' :
			ret = strtol (buf, &s, 10);
			if (s == buf)
				return NULL;
			set_week_number_sun (timeptr, ret);
			buf = s;
			break;
		case 'V' :
			ret = strtol (buf, &s, 10);
			if (s == buf)
				return NULL;
			set_week_number_mon4 (timeptr, ret);
			buf = s;
			break;
		case 'W' :
			ret = strtol (buf, &s, 10);
			if (s == buf)
				return NULL;
			set_week_number_mon (timeptr, ret);
			buf = s;
			break;
		case 'x' :
			s = strptime (buf, "%Y:%m:%d", timeptr);
			if (s == NULL)
				return NULL;
			buf = s;
			break;
		case 'y' :
			ret = strtol (buf, &s, 10);
			if (s == buf)
				return NULL;
			if (ret < 70)
				timeptr->tm_year = 100 + ret;
			else
				timeptr->tm_year = ret;
			buf = s;
			break;
		case 'Y' :
			ret = strtol (buf, &s, 10);
			if (s == buf)
				return NULL;
			timeptr->tm_year = ret - tm_year_base;
			buf = s;
			break;
		case 'Z' :
			abort ();
		case '\0' :
			--format;
			/* FALLTHROUGH */
		case '%' :
			if (*buf == '%')
				++buf;
			else
				return NULL;
			break;
		default :
			if (*buf == '%' || *++buf == c)
				++buf;
			else
				return NULL;
			break;
			}
		} else {
			if (*buf == c)
				++buf;
			else
				return NULL;
		}
	}
	return (char *)buf;
}


//-----------------------------------------------------------------------------

static Status wtime_Init()
{
	LatchInitialSystemTime();
	return INFO::OK;
}
