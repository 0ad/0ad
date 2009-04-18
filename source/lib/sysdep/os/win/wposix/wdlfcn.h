#ifndef INCLUDED_WDLFCN
#define INCLUDED_WDLFCN

//
// <dlfcn.h>
//

// these have no meaning for the Windows GetProcAddress implementation,
// so they are ignored but provided for completeness.
#define RTLD_LAZY   0x01
#define RTLD_NOW    0x02
#define RTLD_GLOBAL 0x04	// semantics are unsupported, so complain if set.
#define RTLD_LOCAL  0x08

extern int dlclose(void* handle);
extern char* dlerror(void);
extern void* dlopen(const char* so_name, int flags);
extern void* dlsym(void* handle, const char* sym_name);

#endif	// #ifndef INCLUDED_WDLFCN
