#if OS_WIN
# include "lib/sysdep/os/win/wposix/wutsname.h"
#else
# include <sys/utsname.h>
#endif

#include "posix_errno.h"	// for user convenience
