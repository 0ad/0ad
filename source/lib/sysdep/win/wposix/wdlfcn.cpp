#include "precompiled.h"
#include "wdlfcn.h"

#include "lib/path_util.h"
#include "wposix_internal.h"


static HMODULE HMODULE_from_void(void* handle)
{
	return (HMODULE)handle;
}

static void* void_from_HMODULE(HMODULE hModule)
{
	return (void*)hModule;
}


int dlclose(void* handle)
{
	BOOL ok = FreeLibrary(HMODULE_from_void(handle));
	WARN_RETURN_IF_FALSE(ok);
	return 0;
}


char* dlerror(void)
{
	return 0;
}


void* dlopen(const char* so_name, int flags)
{
	debug_assert(!(flags & RTLD_GLOBAL));

	// if present, strip .so extension; add .dll extension
	char dll_name[MAX_PATH];
	strcpy_s(dll_name, ARRAY_SIZE(dll_name)-5, so_name);
	char* ext = (char*)path_extension(dll_name);
	if(ext[0] == '\0')	// no extension
		strcat(dll_name, ".dll");	// safe
	else	// need to replace extension
		SAFE_STRCPY(ext, "dll");

	HMODULE hModule = LoadLibrary(dll_name);
	debug_assert(hModule);
	return void_from_HMODULE(hModule);
}


void* dlsym(void* handle, const char* sym_name)
{
	HMODULE hModule = HMODULE_from_void(handle);
	void* sym = GetProcAddress(hModule, sym_name);
	debug_assert(sym);
	return sym;
}
