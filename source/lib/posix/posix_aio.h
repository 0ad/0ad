// despite the comment in wposix.h about not using Windows headers for
// POSIX declarations, this one is harmless (no incompatible declarations)
// and can safely be used on Windows as well.
#include <fcntl.h>

#if OS_WIN
# include "lib/sysdep/os/win/wposix/waio.h"
#else
# include <aio.h>
#endif

#include "posix_errno.h"	// for user convenience
