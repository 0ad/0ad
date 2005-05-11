#ifndef DEBUG_H_INCLUDED
#define DEBUG_H_INCLUDED

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
// assert
//

// notify the user that an assertion failed.
// displays a stack trace with local variables on Windows.
// return values: 0 = continue; 1 = suppress; 2 = break
// .. or exits the program if the user so chooses.
extern int debug_assert_failed(const char* source_file, int line, const char* assert_expr);

// recommended use: assert2(expr && "descriptive string")
#define assert2(expr)\
STMT(\
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
)


//
// output
//

// output to the debugger (may take ~1 ms!)
extern void debug_out(const char* fmt, ...);

// log to memory buffer (fast)
#define MICROLOG debug_microlog
extern void debug_microlog(const wchar_t* fmt, ...);

#define debug_warn(str) assert2(0 && (str))


//
// breakpoints
//

// 
//#define debug_break()


// sometimes mmgr's 'fences' (making sure padding before and after the
// allocation remains intact) aren't enough to catch hard-to-find
// memory corruption bugs. another tool is to trigger a debug exception
// when the later to be corrupted variable is accessed; the problem should
// then become apparent.
// the VC++ IDE provides such 'breakpoints', but 


// values chosen to match IA-32 bit defs, so compiler can optimize.
// this isn't required, it'll work regardless.
enum DbgBreakType
{
	DBG_BREAK_CODE       = 0,
	DBG_BREAK_DATA_WRITE = 1,
	DBG_BREAK_DATA       = DBG_BREAK_DATA_WRITE|2
};

extern int debug_set_break(void* addr, DbgBreakType type);


//
// memory
//

// independent of mmgr, endeavor to 

extern void debug_check_heap(void);


//
// symbol access
//

const size_t DBG_SYMBOL_LEN = 1000;
const size_t DBG_FILE_LEN = 100;

extern void* debug_get_nth_caller(uint n);

extern int debug_resolve_symbol(void* ptr_of_interest, char* sym_name, char* file, int* line);


//
// crash notification
//

extern int debug_write_crashlog(const char* file, const wchar_t* header, void* context);


#endif	// #ifndef DEBUG_H_INCLUDED
