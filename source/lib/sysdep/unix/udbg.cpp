#include "precompiled.h"

#include "lib.h"
#include "timer.h"
#include "sysdep/sysdep.h"

#include <sys/types.h>
#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#ifdef OS_LINUX
#define GNU_SOURCE
#include <dlfcn.h>
#include <execinfo.h>
#endif

#ifdef OS_LINUX
# define DEBUGGER_WAIT 3
# define DEBUGGER_CMD "gdb"
# define DEBUGGER_ARG_FORMAT "--pid=%d"
# define DEBUGGER_BREAK_AFTER_WAIT 0
#else
# error "port"
#endif

#ifndef NDEBUG

#include "bfd.h"
#include <cxxabi.h>

#ifndef bfd_get_section_size
#define bfd_get_section_size bfd_get_section_size_before_reloc
#endif

#define PROFILE_RESOLVE_SYMBOL 0

// Hard-coded - yuck :P
#ifdef TESTING
#define EXE_NAME "ps_test"
#else
#define EXE_NAME "ps_dbg"
#endif

struct symbol_file_context
{
	asymbol **syms;
	bfd *abfd;
};
symbol_file_context ps_dbg_context;
bool udbg_initialized=false;

struct symbol_lookup_context
{
	symbol_file_context *file_ctx;

	bfd_vma address;
	const char* symbol;
	const char* filename;
	uint line;
	
	bool found;
};

#endif

void unix_debug_break()
{
	kill(getpid(), SIGTRAP);
}

/*
Start the debugger and tell it to attach to the current process/thread
*/
static void launch_debugger()
{
	pid_t orgpid=getpid();
	pid_t ret=fork();
	if (ret == 0)
	{
		// Child Process: exec() gdb (Debugger), set to attach to old fork
		char buf[16];
		snprintf(buf, 16, DEBUGGER_ARG_FORMAT, orgpid);
		
		int ret=execlp(DEBUGGER_CMD, DEBUGGER_CMD, buf, NULL);
		// In case of success, we should never get here anyway, though...
		if (ret != 0)
		{
			perror("Debugger launch failed");
		}
	}
	else if (ret > 0)
	{
		// Parent (original) fork:
		sleep(DEBUGGER_WAIT);
	}
	else // fork error, ret == -1
	{
		perror("Debugger launch: fork failed");
	}
}


// notify the user that an assertion failed.
// returns one of FailedAssertUserChoice or exits the program.
int debug_assert_failed(const char *file, int line, const char *expr)
{
	printf("%s:%d: Assertion `%s' failed.\n", file, line, expr);
	do
	{
		printf("(B)reak, Launch (D)ebugger, (C)ontinue, (S)uppress or (A)bort? ");
		// TODO Should have some kind of timeout here.. in case you're unable to
		// access the controlling terminal (As might be the case if launched
		// from an xterm and in full-screen mode)
		int c=getchar();
		if (c == EOF) // I/O Error
			return 2;
		c=tolower(c);
		switch (c)
		{
		case 'd':
			launch_debugger();
			// fall through

		case 'b':
			return ASSERT_BREAK;
		case 'c':
			return ASSERT_CONTINUE;
		case 's':
			return ASSERT_SUPPRESS;
		case 'a':
			abort();
		default:
			continue;
		}
	} while (false);
}

void* debug_get_nth_caller(uint n)
{
	// bt[0] == debug_get_nth_caller
	// bt[1] == caller of get_nth_caller
	// bt[2] == 1:st caller (n==1)
	void *bt[n+2];
	int bt_size;
	
	bt_size=backtrace(bt, n+2);
	// oops - out of stack frames
	assert2((bt_size >= n+2) && "Hrmm.. Not enough stack frames to backtrace!");
	return bt[n+1]; // n==1 => bt[2], and so forth
}

static int slurp_symtab(symbol_file_context *ctx)
{
	bfd *abfd=ctx->abfd;
	asymbol ***syms=&ctx->syms;
	long symcount;
	unsigned int size=0;

	if ((bfd_get_file_flags (abfd) & HAS_SYMS) == 0)
	{
		printf("slurp_symtab(): Huh? Has no symbols...\n");
		return -1;
	}

	symcount = bfd_read_minisymbols (abfd, FALSE, (void **)syms, &size);
	if (symcount == 0)
		symcount = bfd_read_minisymbols (abfd, TRUE /* dynamic */, (void **)syms, &size);

	if (symcount < 0)
	{
		bfd_perror("slurp_symtab");
		return -1;
	}
	
	return 0;
}

static int read_symbols(const char *file_name, symbol_file_context *ctx)
{
	char **matching=NULL;
	
	ONCE(bfd_init());

	ctx->abfd = bfd_openr (file_name, NULL);
	if (ctx->abfd == NULL)
	{
		bfd_perror("udbg.cpp: bfd_openr");
		return -1;
	}

	if (! bfd_check_format_matches (ctx->abfd, bfd_object, &matching))
	{
		if (bfd_get_error () == bfd_error_file_ambiguously_recognized)
		{
			printf("Error reading symbols from %s: ambiguous format\n");
			while (*matching)
				printf("\tPotential matching format: %s\n", *matching++);
			free(matching);
		}
		else
		{
			bfd_perror("bfd_check_format_matches");
		}
		return -1;
	}

	int res=slurp_symtab(ctx);
	if (res == 0)
		return res;
	else
	{
		bfd_perror("udbg.cpp: slurp_symtab");
		bfd_close(ctx->abfd);
		return -1;
	}
}

void udbg_init(void)
{
	if (read_symbols(EXE_NAME, &ps_dbg_context)==0)
		udbg_initialized=true;

#if PROFILE_RESOLVE_SYMBOL
	{
		TIMER(udbg_init_benchmark)
		char symbol[DBG_SYMBOL_LEN];
		char file[DBG_FILE_LEN];
		int line;
		debug_resolve_symbol(debug_get_nth_caller(3), symbol, file, &line);
		printf("%s (%s:%d)\n", symbol, file, line);
		for (int i=0;i<1000000;i++)
		{
			debug_resolve_symbol(debug_get_nth_caller(1), symbol, file, &line);
		}
	}
#endif
}

static void find_address_in_section (bfd *abfd, asection *section, void *data)
{
	symbol_lookup_context *ctx=(symbol_lookup_context *)data;
	asymbol **syms=ctx->file_ctx->syms;

	bfd_vma pc=ctx->address;
	bfd_vma vma;
	bfd_size_type size;
	
	if (ctx->found) return;

	if ((bfd_get_section_flags (abfd, section) & SEC_ALLOC) == 0)
		return;

	vma = bfd_get_section_vma (abfd, section);
	if (pc < vma)
		return;

	size = bfd_get_section_size (section);
	if (pc >= vma + size)
		return;

	ctx->found = bfd_find_nearest_line (abfd, section, syms,
				pc - vma, &ctx->filename, &ctx->symbol, &ctx->line);
}

// BFD functions perform allocs with real malloc - we need to free that data
#include "nommgr.h"
int demangle_buf(char *buf, const char *symbol, size_t n)
{
	int status=0;
	char *alloc=NULL;
	if (symbol == NULL || *symbol == '\0')
	{
		symbol = "??";
		status = -1;
	}
	else
	{
		alloc=abi::__cxa_demangle(symbol, NULL, NULL, &status);
	}
	// status is 0 on success and a negative value on failure
	if (status == 0)
		symbol=alloc;
	strncpy(buf, symbol, n);
	buf[n-1]=0;
	if (alloc)
		free(alloc);
}

int debug_resolve_symbol_dladdr(void *ptr, char* sym_name, char* file, int* line)
{
	Dl_info syminfo;
	
	int res=dladdr(ptr, &syminfo);
	if (res == 0) return -1;
	
	if (syminfo.dli_sname)
		demangle_buf(sym_name, syminfo.dli_sname, DBG_SYMBOL_LEN);
	else
	{
		snprintf(sym_name, DBG_SYMBOL_LEN, "0x%08x", ptr);
		sym_name[DBG_SYMBOL_LEN-1]=0;
	}
	
	strncpy(file, syminfo.dli_fname, DBG_FILE_LEN);
	file[DBG_FILE_LEN-1]=0;
	
	*line=0;
	return 0;
}

int debug_resolve_symbol(void* ptr_of_interest, char* sym_name, char* file, int* line)
{
	ONCE(udbg_init());

	// We use our default context - in the future we could do something like
	// mapping library -> file context to support more detailed reporting on
	// external libraries
	symbol_file_context *file_ctx=&ps_dbg_context;
	bfd *abfd=file_ctx->abfd;

	// Reset here if we fail later on
	*sym_name=0;
	*file=0;
	*line=0;

	if (!udbg_initialized)
		return debug_resolve_symbol_dladdr(ptr_of_interest, sym_name, file, line);
	
	symbol_lookup_context ctx;
	memset(&ctx, 0, sizeof(ctx));
	ctx.address=(bfd_vma)ptr_of_interest;
	ctx.file_ctx=file_ctx;

	bfd_map_over_sections (abfd, find_address_in_section, &ctx);
	
	// This will happen for addresses in external files. What one *could* do
	// here is to figure out the originating library file and load that through
	// BFD... but how often will external libraries have debugging info? really?
	// At least attempt to find out the symbol name through dladdr.
	if (!ctx.found)
		return debug_resolve_symbol_dladdr(ptr_of_interest, sym_name, file, line);

	demangle_buf(sym_name, ctx.symbol, DBG_SYMBOL_LEN);

	if (ctx.filename != NULL)
	{
		const char *h;

		h = strrchr (ctx.filename, '/');
		if (h != NULL)
			ctx.filename = h + 1;
	}
	
	strncpy(file, ctx.filename, DBG_FILE_LEN);
	file[DBG_FILE_LEN]=0;
	
	*line = ctx.line;
	
	return 0;
}
#include "mmgr.h"

int debug_write_crashlog(const char* file, wchar_t* header, void* context)
{
	// TODO: Do this properly. (I don't know what I'm doing; I just
	// know that this function is required in order to compile...)

	abort();
}


void debug_check_heap()
{
	// TODO: Do this properly. (I don't know what I'm doing; I just
	// know that this function is required in order to compile...)
}



void debug_printf(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	vprintf(fmt, args);
	va_end(args);
	fflush(stdout);
}