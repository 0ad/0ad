
#ifdef _WIN32

#include "sysdep/win/wposix.h"
#include "sysdep/win/win.h"

#else

#include <sys/types.h>
#include <limits.h>
#include <unistd.h>
#include <sys/time.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/mman.h>
#include <aio.h>
#include <errno.h>
#include <dirent.h>
#include <sys/utsname.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#endif	// #ifdef _WIN32 else