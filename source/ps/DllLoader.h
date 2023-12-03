/* Copyright (C) 2023 Wildfire Games.
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

#ifndef INCLUDED_DLLLOADER
#define INCLUDED_DLLLOADER

#include "ps/Errors.h"
#include "ps/CLogger.h"
#include "ps/CStr.h"

ERROR_GROUP(DllLoader);
ERROR_TYPE(DllLoader, DllNotLoaded);
ERROR_TYPE(DllLoader, SymbolNotFound);

class DllLoader
{
public:
	/**
	 * Prepare the DLL loader. Does no actual work.
	 *
	 * @param name base name of the library (from which we'll derive
	 *  "name.dll", "libname_dbg.so", etc). Pointer must remain valid for
	 *  this object's lifetime (which is fine if you just use a string literal).
	 * @param loadErrorLogMethod Allows to set the CLogger log level that is
	 *  used when the DllLoader reports loading errors.
	 */
	DllLoader(const char* name, CLogger::ELogMethod loadErrorLogMethod = CLogger::Error);

	~DllLoader();

	/**
	 * Attempt to load and initialise the library, if not already. Can be harmlessly
	 * called multiple times. Returns false if unsuccessful.
	 */
	bool LoadDLL();

	/**
	 * Check whether the library has been loaded successfully. Returns false
	 * before {@link #LoadDLL} has been called; otherwise returns the same as
	 * LoadDLL did.
	 */
	bool IsLoaded() const;

	/**
	 * Unload the library, if it has been loaded already. (Usually not needed,
	 * since the destructor will unload it.)
	 */
	void Unload();

	/**
	 * Attempt to load a named symbol from the library. If {@link #IsLoaded} is
	 * false, throws PSERROR_DllLoader_DllNotLoaded. If it cannot load the
	 * symbol, throws PSERROR_DllLoader_SymbolNotFound. In both cases, sets fptr
	 * to NULL. Otherwise, fptr is set to point to the loaded function.
	 *
	 * @throws PSERROR_DllLoader
	 */
	template <typename T>
	void LoadSymbol(const char* name, T& fptr) const;

	/**
	 * Override the build-time setting of the directory to search for libraries.
	 */
	static void OverrideLibdir(const char* libdir);

	static CStr GenerateFilename(const CStr& name, const CStr& suffix, const CStr& extension);

private:
	// Typeless version - the public LoadSymbol hides the slightly ugly
	// casting from users.
	void LoadSymbolInternal(const char* name, void** fptr) const;

	void LogLoadError(const char* errors);

	const char* m_Name;
	void* m_Handle;
	CLogger::ELogMethod m_LoadErrorLogMethod;
};

template <typename T>
void DllLoader::LoadSymbol(const char* name, T& fptr) const
{
	LoadSymbolInternal(name, (void**)&fptr);
}

#endif // INCLUDED_DLLLOADER
