// lightweight header that defines POSIX types (e.g. ssize_t and int8_t,
// which is also provided by C99) without pulling in all POSIX declarations.
// included from lib/types.h in place of posix.h; this helps avoid conflicts
// due to incompatible winsock definitions.

#ifdef _WIN32
# include "sysdep/win/wposix_types.h"
#else
# include <stdint.h>
#endif	// #ifdef _WIN32
