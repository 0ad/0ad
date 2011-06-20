/* Copyright (C) 2011 Wildfire Games.
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

#include "lib/timer.h"
#include "lib/posix/posix_dlfcn.h"
#include "ps/CStr.h"
#include "ps/CLogger.h"
#include "ps/GameSetup/Config.h"

static void* const HANDLE_UNAVAILABLE = (void*)-1;

// directory to search for libraries (optionally set by --libdir at build-time,
// optionally overridden by -libdir at run-time in the test executable);
// if we don't have an explicit libdir then the linker will look in DT_RUNPATH
// (which we set to $ORIGIN) to find it in the executable's directory
#ifdef INSTALLED_LIBDIR
static CStr g_Libdir = STRINGIZE(INSTALLED_LIBDIR);
#else
static CStr g_Libdir = "";
#endif

// note: on Linux, lib is prepended to the SO file name
#if OS_UNIX
static CStr prefix = "lib";
#else
static CStr prefix = "";
#endif

// we usually want to use the same debug/release build type as the
// main executable (the library should also be efficient in release builds and
// allow easy symbol access in debug builds). however, that version of the
// library might be missing, so we check for both.
// this works because the interface is binary-compatible.
#ifndef NDEBUG
static CStr suffixes[] = { "_dbg", "" };	// (order matters)
#else
static CStr suffixes[] = { "", "_dbg" };
#endif

// NB: our Windows dlopen() function changes the extension to .dll
static CStr extensions[] = {
	".so",
#if OS_MACOSX
	".dylib"	// supported by OS X dlopen
#endif
};

// (This class is currently only used by 'Collada' and 'AtlasUI' which follow
// the naming/location convention above - it'll need to be changed if we want
// to support other DLLs.)

static CStr GenerateFilename(const CStr& name, const CStr& suffix, const CStr& extension)
{
	CStr n;
	if (!g_Libdir.empty())
		n = g_Libdir + "/";
	n += prefix + name + suffix + extension;
	return n;
}



// @param name base name of the library (excluding prefix/suffix/extension)
// @param errors receives descriptions of any and all errors encountered
// @return valid handle or 0
static void* LoadAnyVariant(const CStr& name, std::stringstream& errors)
{
	for(size_t idxSuffix = 0; idxSuffix < ARRAY_SIZE(suffixes); idxSuffix++)
	{
		for(size_t idxExtension = 0; idxExtension < ARRAY_SIZE(extensions); idxExtension++)
		{
			CStr filename = GenerateFilename(name, suffixes[idxSuffix], extensions[idxExtension]);

			// we don't really care when relocations take place, but one of
			// {RTLD_NOW, RTLD_LAZY} must be specified. go with the former because
			// it is safer and matches the Windows load behavior.
			const int flags = RTLD_LOCAL|RTLD_NOW;
			void* handle = dlopen(filename.c_str(), flags);
			if(handle)
				return handle;
			else
				errors << "dlopen(" << filename << ") failed: " << dlerror() << "; ";
		}
	}

	return 0;	// none worked
}


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
		TIMER(L"LoadDLL");

		std::stringstream errors;
		m_Handle = LoadAnyVariant(m_Name, errors);
		if(!m_Handle)	// (only report errors if nothing worked)
		{
			LOGERROR(L"DllLoader: %hs", errors.str().c_str());
			m_Handle = HANDLE_UNAVAILABLE;
		}
	}

	return (m_Handle != HANDLE_UNAVAILABLE);
}

void DllLoader::Unload()
{
	if(!IsLoaded())
		return;

	dlclose(m_Handle);
	m_Handle = 0;
}

void DllLoader::LoadSymbolInternal(const char* name, void** fptr) const
{
	if(!IsLoaded())
	{
		debug_warn(L"Loading symbol from invalid DLL");
		*fptr = NULL;
		throw PSERROR_DllLoader_DllNotLoaded();
	}

	*fptr = dlsym(m_Handle, name);
	if(*fptr == NULL)
		throw PSERROR_DllLoader_SymbolNotFound();
}

void DllLoader::OverrideLibdir(const char* libdir)
{
	g_Libdir = libdir;
}
