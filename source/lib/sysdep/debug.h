//
// logging
//

// output to the debugger (may take ~1 ms!)
extern void debug_out(const char* fmt, ...);

// log to memory buffer (fast)
#define MICROLOG debug_microlog
extern void debug_microlog(const wchar_t *fmt, ...);


//
// crash notification
//

// notify the user that an assertion failed.
// displays a stack trace with local variables on Windows.
// return values: 0 = continue; 1 = suppress; 2 = break
// .. or exits the program if the user so chooses.
extern int debug_assert_failed(const char* source_file, int line, const char* assert_expr);

extern int debug_write_crashlog(const char* file, wchar_t* header, void* context);



extern void debug_check_heap();

extern void debug_break();
