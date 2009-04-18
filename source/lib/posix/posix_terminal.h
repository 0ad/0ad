#if OS_WIN
# include "lib/sysdep/os/win/wposix/wterminal.h"
#else
# include <sys/ioctl.h>
#endif

#include "posix_errno.h"	// for user convenience
