/**
 * see rationale in wposix.h.
 * prevent subsequent includes of CRT headers (e.g. <io.h>) from
 * interfering with previous declarations made by wposix headers (e.g. open).
 * this is accomplished by #defining their include guards.
 * note: #include "crt_posix.h" can undo the effects of this header and
 * pull in those headers.
 **/

#ifndef _INC_IO	// <io.h> include guard
# define _INC_IO
# define WPOSIX_DEFINED_IO_INCLUDE_GUARD
#endif
#ifndef _INC_DIRECT	// <direct.h> include guard
# define _INC_DIRECT
# define WPOSIX_DEFINED_DIRECT_INCLUDE_GUARD
#endif
