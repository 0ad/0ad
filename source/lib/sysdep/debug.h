#ifndef DEBUG_H__
#define DEBUG_H__

// we need to include the platform-specific version here, so it can
// define debug_break. if it can be implemented as a macro (e.g. on ia32),
// the debugger will break directly at the target, instead of one function
// below it as with a conventional implementation.
#ifdef _WIN32
# include "win/wdbg.h"
#else
# include "unix/udbg.h"
#endif


//
// logging
//

// output to the debugger (may take ~1 ms!)
extern void debug_out(const char* fmt, ...);

// log to memory buffer (fast)
#define MICROLOG debug_microlog
extern void debug_microlog(const wchar_t* fmt, ...);



const size_t DBG_SYMBOL_LEN = 1000;
const size_t DBG_FILE_LEN = 100;

extern void* debug_get_nth_caller(uint n);

extern int debug_resolve_symbol(void* ptr_of_interest, char* sym_name, char* file, int* line);



//
// crash notification
//

// notify the user that an assertion failed.
// displays a stack trace with local variables on Windows.
// return values: 0 = continue; 1 = suppress; 2 = break
// .. or exits the program if the user so chooses.
extern int debug_assert_failed(const char* source_file, int line, const char* assert_expr);

extern int debug_write_crashlog(const char* file, const wchar_t* header, void* context);


extern void debug_check_heap(void);



// superassert
// recommended use: assert2(expr && "descriptive string")
#define assert2(expr)\
{\
	static int suppress__ = 0;\
	if(!suppress__ && !(expr))\
	switch(debug_assert_failed(__FILE__, __LINE__, #expr))\
	{\
	case 1:\
		suppress__ = 1;\
		break;\
	case 2:\
		debug_break();\
		break;\
	}\
}


#endif	// #ifndef DEBUG_H__
