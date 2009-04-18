/**
 * =========================================================================
 * File        : fat_time.cpp
 * Project     : 0 A.D.
 * Description : timestamp conversion: DOS FAT <-> Unix time_t
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#include "precompiled.h"
#include "fat_time.h"

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

	u16 fat_time = 0;
	fat_time |= u16(t->tm_sec/2);		// 5
	fat_time |= u16(t->tm_min) << 5;	// 6
	fat_time |= u16(t->tm_hour) << 11;	// 5

	u16 fat_date = 0;
	fat_date |= u16(t->tm_mday);			// 5
	fat_date |= u16(t->tm_mon+1) << 5;		// 4
	fat_date |= u16(t->tm_year-80) << 9;	// 7

	u32 fat_timedate = u32_from_u16(fat_date, fat_time);
	return fat_timedate;
}
