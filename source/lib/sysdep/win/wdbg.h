#ifndef WDBG_H__
#define WDBG_H__

#include "lib/types.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _MSC_VER
# define debug_break() __asm { int 3 }
#else
# error "port this or define to implementation function"
#endif


// TODO: remove this from here, and make all the exception debugging stuff nicer
struct _EXCEPTION_POINTERS;
typedef struct _EXCEPTION_POINTERS* PEXCEPTION_POINTERS;
extern int debug_main_exception_filter(unsigned int code, PEXCEPTION_POINTERS ep);


#ifdef __cplusplus
}
#endif

#endif	// #ifndef WDBG_H__
