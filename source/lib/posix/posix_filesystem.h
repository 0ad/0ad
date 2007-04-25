#if OS_WIN
# include "lib/sysdep/win/wposix/wfilesystem.h"
#else
# include <sys/stat.h>
# include <sys/types.h>
# include <dirent.h>
#endif

#include "posix_errno.h"	// for user convenience

#if !HAVE_MKDIR
extern int mkdir(const char* path, mode_t mode);
#endif
