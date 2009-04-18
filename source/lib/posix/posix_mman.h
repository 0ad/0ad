#if OS_WIN
# include "lib/sysdep/os/win/wposix/wmman.h"
#else
# include <sys/mman.h>
#endif

#include "posix_errno.h"	// for user convenience
