#ifndef __DEBUG_H__
#define __DEBUG_H__

#ifdef __cplusplus
extern "C" {
#endif


/*
 * exception handler and assert with stack trace
 * (including variable / parameter types and values)
 * shown in a dialog, which offers
 *   continue, break, suppress (ignore this assert), and exit
 */

/* register exception handler */
extern void dbg_init_except_handler();

/* recommended use: assert2(expr && "descriptive string") */
#define assert2(expr)\
{\
	static int suppress;\
	if(!suppress && !expr)\
		switch(show_assert_dlg(__FILE__, __LINE__, #expr))\
		{\
		case 1:\
			suppress = 1;\
			break;\
\
		case 2:\
			__asm { int 3 }		/* x86 specific; Win alternative: DebugBreak */\
			break;\
		}\
}

extern int show_assert_dlg(char* file, int line, char* expr);


#ifdef __cplusplus
}
#endif

#endif	// #ifndef __DEBUG_H__
