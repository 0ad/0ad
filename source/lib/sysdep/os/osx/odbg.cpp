// note: the BFD stuff *could* be used on other platforms, if we saw the
// need for it.

#include "precompiled.h"

#include "lib/sysdep/sysdep.h"
#include "lib/debug.h"

void* debug_GetCaller(void* UNUSED(context), const char* UNUSED(lastFuncToSkip))
{
	return NULL;
}

LibError debug_DumpStack(wchar_t* UNUSED(buf), size_t UNUSED(max_chars), size_t UNUSED(skip), void* UNUSED(context))
{
	return ERR::NOT_IMPLEMENTED;
}

LibError debug_ResolveSymbol(void* UNUSED(ptr_of_interest), char* UNUSED(sym_name), char* UNUSED(file), int* UNUSED(line))
{
	return ERR::NOT_IMPLEMENTED;
}

void debug_SetThreadName(char const* UNUSED(name))
{
    // Currently unimplemented
}
