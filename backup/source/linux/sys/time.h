#ifndef _bug_time_H
#define _bug_time_H

#include "/usr/include/sys/time.h"

#define __need_time_t
#include <time.h>

#ifndef __timespec_defined
#define __timespec_defined 1
struct timespec {
	__time_t tv_sec;
	long int tv_nsec;
};
#endif		

typedef __time_t time_t;
typedef __clockid_t clockid_t;

#endif
