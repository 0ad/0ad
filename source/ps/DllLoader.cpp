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

#include "precompiled.h"

#include "DllLoader.h"

#include "lib/posix/posix_dlfcn.h"
#include "ps/CStr.h"
#include "ps/CLogger.h"

void* const HANDLE_UNAVAILABLE = (void*)-1;


// TODO Use path_util instead, get the actual path to the ps_dbg exe and append
// the library name.

// note: on Linux, lib is prepended to the SO file name;
// we don't use a path with '/' so the linker will look in DT_RUNPATH
// (which we set to $ORIGIN) to find it in the executable's directory
#if OS_UNIX
static const char* prefix = "lib";
#else
static const char* prefix = "";
#endif
// our SOs export binary-compatible interfaces across debug and release builds,
// but for debugging/performance we prefer to use the same version as the app.
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
			const char* error = dlerror();
			if (error)
				LOG(CLogger::Error, L"", L"dlopen error: %hs", error);
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
		debug_warn(L"Loading symbol from invalid DLL");
		*fptr = NULL;
		throw PSERROR_DllLoader_DllNotLoaded();
	}

	*fptr = dlsym(m_Handle, name);

	if (*fptr == NULL)
		throw PSERROR_DllLoader_SymbolNotFound();
}
