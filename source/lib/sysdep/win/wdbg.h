#ifndef WDBG_H__
#define WDBG_H__

#include "lib/types.h"

#ifdef __cplusplus
extern "C" {
#endif

// TODO: remove this from here, and make all the exception debugging stuff nicer
struct _EXCEPTION_POINTERS;
typedef struct _EXCEPTION_POINTERS* PEXCEPTION_POINTERS;
extern int debug_main_exception_filter(unsigned int code, PEXCEPTION_POINTERS ep);

extern void* wdbg_get_nth_caller(uint n);
extern int wdbg_resolve_symbol(void* ptr_of_interest, char* sym_name, char* file, int* line);


#ifdef __cplusplus
}
#endif

#endif	// #ifndef WDBG_H__
