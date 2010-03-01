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
 * timestamp conversion: DOS FAT <-> Unix time_t
 */

#include "precompiled.h"
#include "lib/fat_time.h"

#include <ctime>

#include "lib/bits.h"


time_t time_t_from_FAT(u32 fat_timedate)
{
	const u32 fat_time = bits(fat_timedate, 0, 15);
	const u32 fat_date = bits(fat_timedate, 16, 31);

	struct tm t;							// struct tm format:
	t.tm_sec   = bits(fat_time, 0,4) * 2;	// [0,59]
	t.tm_min   = bits(fat_time, 5,10);		// [0,59]
	t.tm_hour  = bits(fat_time, 11,15);		// [0,23]
	t.tm_mday  = bits(fat_date, 0,4);		// [1,31]
	t.tm_mon   = bits(fat_date, 5,8) - 1;	// [0,11]
	t.tm_year  = bits(fat_date, 9,15) + 80;	// since 1900
	t.tm_isdst = -1;	// unknown - let libc determine

	// otherwise: totally bogus, and at the limit of 32-bit time_t
	debug_assert(t.tm_year < 138);

	time_t ret = mktime(&t);
	debug_assert(ret != (time_t)-1);	// mktime shouldn't fail
	return ret;
}


u32 FAT_from_time_t(time_t time)
{
	// (values are adjusted for DST)
	struct tm* t = localtime(&time);

	const u16 fat_time = u16(
		(t->tm_sec/2) |		    // 5
		(u16(t->tm_min) << 5) | // 6
		(u16(t->tm_hour) << 11)	// 5
	);

	const u16 fat_date = u16(
		(t->tm_mday) |            // 5
		(u16(t->tm_mon+1) << 5) | // 4
		(u16(t->tm_year-80) << 9) // 7
	);

	u32 fat_timedate = u32_from_u16(fat_date, fat_time);
	return fat_timedate;
}
