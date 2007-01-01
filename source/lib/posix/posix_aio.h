#if OS_WIN
# include "lib/sysdep/win/wposix/waio.h"
#else
# include <fcntl.h>
# include <aio.h>
#endif

#include "posix_errno.h"	// for user convenience
