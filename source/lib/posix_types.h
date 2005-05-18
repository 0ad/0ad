// lightweight header that defines POSIX types (e.g. ssize_t and int8_t,
// which is also provided by C99) without pulling in all POSIX declarations.
// included from lib/types.h in place of posix.h; this helps avoid conflicts
// due to incompatible winsock definitions.

#include <wchar.h>
#include <sys/types.h>
#include <stddef.h>

#ifdef _WIN32
# include "sysdep/win/wposix_types.h"
#else
// unix/linux/glibc/gcc says that this macro has to be defined when including
// stdint.h from C++ for stdint.h to define SIZE_MAX and friends
# ifndef __STDC_LIMIT_MACROS
#  define __STDC_LIMIT_MACROS
# endif
# include <stdint.h>
#endif	// #ifdef _WIN32
