#include "precompiled.h"
#include "wdlfcn.h"

#include "lib/os_path.h"
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

	OsPath path = fs::change_extension(so_name, ".dll");
	HMODULE hModule = LoadLibrary(path.external_directory_string().c_str());
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
