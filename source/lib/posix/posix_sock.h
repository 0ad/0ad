#if OS_WIN
# include "lib/sysdep/os/win/wposix/wsock.h"
#else
# include <sys/socket.h>
# include <netdb.h>
# include <netinet/in.h>
# include <netinet/tcp.h>
# include <arpa/inet.h>
#endif

#include "posix_errno.h"	// for user convenience
