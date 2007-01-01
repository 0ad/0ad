#if OS_WIN
# include "lib/sysdep/win/wposix/wdlfcn.h"
#else
# include <dlfcn.h>
#endif

#include "posix_errno.h"	// for user convenience
