#ifndef WDBG_H__
#define WDBG_H__

#include "win.h"

#ifdef __cplusplus
extern "C" {
#endif


// assert with stack trace (including variable / parameter types and values)
// shown in a dialog, which offers
//   continue, break, suppress (ignore this assert), and exit

// recommended use: assert2(expr && "descriptive string")
#define assert2(expr)\
{\
	static int suppress__;\
	if(!suppress__ && !expr)\
		switch(wdbg_show_assert_dlg(__FILE__, __LINE__, #expr))\
		{\
		case 1:\
			suppress__ = 1;\
			break;\
\
		case 2:\
			win_debug_break();\
			break;\
		}\
}

extern int wdbg_show_assert_dlg(char* file, int line, char* expr);


#ifdef __cplusplus
}
#endif

#endif	// #ifndef WDBG_H__
