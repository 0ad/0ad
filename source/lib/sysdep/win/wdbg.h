/**
 * =========================================================================
 * File        : wdbg.h
 * Project     : 0 A.D.
 * Description : Win32 debug support code and exception handler.
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#ifndef INCLUDED_WDBG
#define INCLUDED_WDBG

#if HAVE_MS_ASM
# define debug_break() __asm { int 3 }
#else
# error "port this or define to implementation function"
#endif


extern void wdbg_set_thread_name(const char* name);


const size_t WDBG_XRR_STORAGE_SIZE = 16;

/**
 * install an SEH handler for the current thread.
 *
 * @param xrrStorage - storage used to hold the SEH handler node that's
 * entered into the TIB's list. must be at least WDBG_XRR_STORAGE_SIZE bytes
 * and remain valid over the lifetime of the thread.
 **/
void wdbg_InstallExceptionHandler(void* xrrStorage);

#endif	// #ifndef INCLUDED_WDBG
