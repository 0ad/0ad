
#ifdef _WIN32

#include "sysdep/win/wposix.h"
#include "sysdep/win/win.h"

#else

#include <sys/types.h>
#include <limits.h>
#include <unistd.h>
#include <sys/time.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <aio.h>
#include <errno.h>
#include <dirent.h>
#include <sys/utsname.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

#endif	// #ifdef _WIN32 else
