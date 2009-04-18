/**
 * see rationale in wposix.h.
 * we need to have CRT functions (e.g. _open) declared correctly
 * but must prevent POSIX functions (e.g. open) from being declared -
 * they would conflict with our wrapper function.
 *
 * since the headers only declare POSIX stuff #if !__STDC__, a solution
 * would be to set this flag temporarily. note that MSDN says that
 * such predefined macros cannot be redefined nor may they change during
 * compilation, but this works and seems to be a fairly common practice.
 **/

// undefine include guards set by no_crt_posix
#if defined(_INC_IO) && defined(WPOSIX_DEFINED_IO_INCLUDE_GUARD)
# undef _INC_IO
#endif
#if defined(_INC_DIRECT) && defined(WPOSIX_DEFINED_DIRECT_INCLUDE_GUARD)
# undef _INC_DIRECT
#endif


#define __STDC__ 1

#include <io.h>             // _open etc.
#include <direct.h>			// _rmdir

#undef __STDC__
#define __STDC__ 0
