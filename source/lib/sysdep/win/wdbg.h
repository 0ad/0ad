#ifndef WDBG_H__
#define WDBG_H__

#include "lib/types.h"

#ifdef __cplusplus
extern "C" {
#endif


extern void* wdbg_get_nth_caller(uint n);
extern int wdbg_resolve_symbol(void* ptr_of_interest, char* sym_name, char* file, int* line);


#ifdef __cplusplus
}
#endif

#endif	// #ifndef WDBG_H__
