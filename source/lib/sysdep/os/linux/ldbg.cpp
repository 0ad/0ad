/* Copyright (C) 2009 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

// note: the BFD stuff *could* be used on other platforms, if we saw the
// need for it.

#include "precompiled.h"

#include "lib/timer.h"
#include "lib/wchar.h"
#include "lib/sysdep/sysdep.h"
#include "lib/debug.h"

#include <stdarg.h>
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <cassert>

#include <bfd.h>
#include <cxxabi.h>

#ifndef bfd_get_section_size
#define bfd_get_section_size bfd_get_section_size_before_reloc
#endif

#define GNU_SOURCE
#include <dlfcn.h>
#include <execinfo.h>

// Hard-coded - yuck :P
// These should only be used as fallbacks
#if defined(TESTING)
#define EXE_NAME "pyrogenesis_test"
#elif defined(NDEBUG)
#define EXE_NAME "pyrogenesis"
#else
#define EXE_NAME "pyrogenesis_dbg"
#endif

#define PROFILE_RESOLVE_SYMBOL 0

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

void* debug_GetCaller(void* UNUSED(context), const wchar_t* UNUSED(lastFuncToSkip))
{
	// bt[0] == this function
	// bt[1] == our caller
	// bt[2] == the first caller they are interested in
	// HACK: we currently don't support lastFuncToSkip (would require debug information),
	// instead just returning the caller of the function calling us
	void *bt[3];
	int bt_size = backtrace(bt, 3);
	if (bt_size < 3)
	    return NULL;
	return bt[2];
}

LibError debug_DumpStack(wchar_t* buf, size_t max_chars, void* UNUSED(context), const wchar_t* UNUSED(lastFuncToSkip))
{
	static const size_t N_FRAMES = 16;
	void *bt[N_FRAMES];
	int bt_size=0;
	wchar_t *bufpos = buf;
	wchar_t *bufend = buf + max_chars;

	bt_size=backtrace(bt, ARRAY_SIZE(bt));

	// Assumed max length of a single print-out
	static const size_t MAX_OUT_CHARS=1024;

	for (size_t i=0;(int)i<bt_size && bufpos+MAX_OUT_CHARS < bufend;i++)
	{
		wchar_t file[DBG_FILE_LEN];
		wchar_t symbol[DBG_SYMBOL_LEN];
		int line;
		int len;
		
		if (debug_ResolveSymbol(bt[i], symbol, file, &line) == 0)
			len = swprintf(bufpos, MAX_OUT_CHARS, L"(0x%08x) %ls:%d %ls\n", bt[i], file, line, symbol);
		else
			len = swprintf(bufpos, MAX_OUT_CHARS, L"(0x%08x)\n", bt[i]);
		
		if (len < 0)
		{
			// MAX_OUT_CHARS exceeded, realistically this was caused by some
			// mindbogglingly long symbol name... replace the end with an
			// ellipsis and a newline
			memcpy(&bufpos[MAX_OUT_CHARS-6], L"...\n", 5*sizeof(wchar_t));
			len = MAX_OUT_CHARS;
		}
		
		bufpos += len;
	}

	return INFO::OK;
}

static int slurp_symtab(symbol_file_context *ctx)
{
	bfd *abfd=ctx->abfd;
	asymbol ***syms=&ctx->syms;
	long symcount;
	int size=0;

	if ((bfd_get_file_flags (abfd) & HAS_SYMS) == 0)
	{
		printf("slurp_symtab(): Huh? Has no symbols...\n");
		return -1;
	}

	size = bfd_get_symtab_upper_bound(abfd);
	if (size < 0)
	{
		bfd_perror("symtab_upper_bound");
		return -1;
	}
	*syms = (asymbol **)malloc(size);
	if (!syms)
		return -1;
	symcount = bfd_canonicalize_symtab(abfd, *syms);

	if (symcount == 0)
		symcount = bfd_read_minisymbols (abfd, FALSE, (void **)syms, (unsigned*)&size);
	if (symcount == 0)
		symcount = bfd_read_minisymbols (abfd, TRUE /* dynamic */, (void **)syms, (unsigned*)&size);

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
			printf("Error reading symbols from %s: ambiguous format\n", file_name);
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

void udbg_bfd_init(void)
{
	char n_path[PATH_MAX];
	const char *exename=n_path;
	if (sys_get_executable_name(n_path, sizeof(n_path)) != INFO::OK)
	{
		debug_printf("sys_get_executable_name didn't work, using hard-coded guess %s.\n", EXE_NAME);
		exename=EXE_NAME;
	}

	debug_printf("udbg_bfd_init: loading symbols from %s.\n", exename);

	if (read_symbols(exename, &ps_dbg_context)==0)
		udbg_initialized=true;

#if PROFILE_RESOLVE_SYMBOL
	{
		TIMER(udbg_init_benchmark)
		char symbol[DBG_SYMBOL_LEN];
		char file[DBG_FILE_LEN];
		int line;
		debug_ResolveSymbol(debug_get_nth_caller(3), symbol, file, &line);
		printf("%s (%s:%d)\n", symbol, file, line);
		for (int i=0;i<1000000;i++)
		{
			debug_ResolveSymbol(debug_get_nth_caller(1), symbol, file, &line);
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
void demangle_buf(char *buf, const char *symbol, size_t n)
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

static LibError debug_resolve_symbol_dladdr(void *ptr, wchar_t* sym_name, wchar_t* file, int* line)
{
	Dl_info syminfo;
	
	int res=dladdr(ptr, &syminfo);
	if (res == 0)
		WARN_RETURN(ERR::FAIL);
	
	if (sym_name)
	{
		if (syminfo.dli_sname)
		{
			char sym_name_buf[DBG_SYMBOL_LEN];
			demangle_buf(sym_name_buf, syminfo.dli_sname, DBG_SYMBOL_LEN);
			wcscpy_s(sym_name, DBG_SYMBOL_LEN, wstring_from_string(sym_name_buf).c_str());
		}
		else
			swprintf_s(sym_name, DBG_SYMBOL_LEN, L"%p", ptr);
	}
	
	if (file)
	{
		wcscpy_s(file, DBG_FILE_LEN, wstring_from_string(syminfo.dli_fname).c_str());
	}
	
	if (line)
	{
		*line=0;
	}
	
	return INFO::OK;
}

LibError debug_ResolveSymbol(void* ptr_of_interest, wchar_t* sym_name, wchar_t* file, int* line)
{
	ONCE(udbg_bfd_init());

	// We use our default context - in the future we could do something like
	// mapping library -> file context to support more detailed reporting on
	// external libraries
	symbol_file_context *file_ctx=&ps_dbg_context;
	bfd *abfd=file_ctx->abfd;

	// Reset here if we fail later on
	if (sym_name)
		*sym_name=0;
	if (file)
		*file=0;
	if (line)
		*line=0;

	if (!udbg_initialized)
		return debug_resolve_symbol_dladdr(ptr_of_interest, sym_name, file, line);
	
	symbol_lookup_context ctx;
	memset(&ctx, 0, sizeof(ctx));
	ctx.address=reinterpret_cast<bfd_vma>(ptr_of_interest);
	ctx.file_ctx=file_ctx;

	bfd_map_over_sections (abfd, find_address_in_section, &ctx);
	
	// This will happen for addresses in external files. What one *could* do
	// here is to figure out the originating library file and load that through
	// BFD... but how often will external libraries have debugging info? really?
	// At least attempt to find out the symbol name through dladdr.
	if (!ctx.found)
		return debug_resolve_symbol_dladdr(ptr_of_interest, sym_name, file, line);

	if (sym_name)
	{
		char sym_name_buf[DBG_SYMBOL_LEN];
		demangle_buf(sym_name_buf, ctx.symbol, DBG_SYMBOL_LEN);
		wcscpy_s(sym_name, DBG_SYMBOL_LEN, wstring_from_string(sym_name_buf).c_str());
	}

	if (file)
	{
		if (ctx.filename != NULL)
		{
			const char *h;
			h = strrchr (ctx.filename, '/');
			if (h != NULL)
				ctx.filename = h + 1;
	
			wcscpy_s(file, DBG_FILE_LEN, wstring_from_string(ctx.filename).c_str());
		}
		else
			wcscpy_s(file, DBG_FILE_LEN, L"none");
	}
	
	if (line)
	{
		*line = ctx.line;
	}
	
	return INFO::OK;
}

void debug_SetThreadName(char const* UNUSED(name))
{
    // Currently unimplemented
}
