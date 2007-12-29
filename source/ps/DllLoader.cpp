#include "precompiled.h"

#include "DllLoader.h"

#include "lib/posix/posix_dlfcn.h"
#include "ps/CStr.h"
#include "ps/CLogger.h"

void* const HANDLE_UNAVAILABLE = (void*)-1;


// TODO Use path_util instead, get the actual path to the ps_dbg exe and append
// the library name.

// note: on Linux, lib is prepended to the SO file name and we need to add ./
// to make dlopen look in the current working directory
#if OS_UNIX
static const char* prefix = "./lib";
#else
static const char* prefix = "";
#endif
// since some of our SOs export a C++ interface, it is critical that
// compiler options are the same between app and SO; therefore,
// we need to go with the debug version in debug builds.
// note: on Windows, the extension is replaced with .dll by dlopen.
#ifndef NDEBUG
static const char* suffix = "_dbg.so";
#else
static const char* suffix = ".so";
#endif

// (This class is currently only used by 'Collada' and 'AtlasUI' which follow
// the naming/location convention above - it'll need to be changed if we want
// to support other DLLs.)

DllLoader::DllLoader(const char* name)
: m_Name(name), m_Handle(0)
{
}

DllLoader::~DllLoader()
{
	if (IsLoaded())
		dlclose(m_Handle);
}

bool DllLoader::IsLoaded() const
{
	return (m_Handle != 0 && m_Handle != HANDLE_UNAVAILABLE);
}

bool DllLoader::LoadDLL()
{
	// first time: try to open the shared object
	// postcondition: m_Handle valid or == HANDLE_UNAVAILABLE.
	if (m_Handle == 0)
	{
		CStr filename = CStr(prefix) + m_Name + suffix;

		// we don't really care when relocations take place, but one of
		// {RTLD_NOW, RTLD_LAZY} must be passed. go with the former because
		// it is safer and matches the Windows load behavior.
		const int flags = RTLD_LOCAL|RTLD_NOW;

		m_Handle = dlopen(filename, flags);

		// open failed (mostly likely SO not found)
		if (! m_Handle)
		{
			char* error = dlerror();
			if (error)
				LOG(CLogger::Error, "", "dlopen error: %s", error);
			m_Handle = HANDLE_UNAVAILABLE;
		}
	}

	return (m_Handle != HANDLE_UNAVAILABLE);
}

void DllLoader::Unload()
{
	if (! IsLoaded())
		return;

	dlclose(m_Handle);
	m_Handle = 0;
}

void DllLoader::LoadSymbolInternal(const char* name, void** fptr) const
{
	if (! IsLoaded())
	{
		debug_warn("Loading symbol from invalid DLL");
		*fptr = NULL;
		throw PSERROR_DllLoader_DllNotLoaded();
	}

	*fptr = dlsym(m_Handle, name);

	if (*fptr == NULL)
		throw PSERROR_DllLoader_SymbolNotFound();
}
