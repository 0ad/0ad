/* Copyright (C) 2018 Wildfire Games.
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

#ifndef INCLUDED_MODINSTALLER
#define INCLUDED_MODINSTALLER

#include "CStr.h"
#include "lib/file/vfs/vfs.h"
#include "scriptinterface/ScriptInterface.h"

#include <vector>

/**
 * Install a mod into the mods directory.
 */
class CModInstaller
{
public:
	enum ModInstallationResult
	{
		SUCCESS,
		FAIL_ON_VFS_MOUNT,
		FAIL_ON_MOD_LOAD,
		FAIL_ON_PARSE_JSON,
		FAIL_ON_EXTRACT_NAME,
		FAIL_ON_MOD_MOVE
	};

	/**
	 * Initialise the mod installer for processing the given mod.
	 *
	 * @param modsdir path to the data directory that contains mods
	 * @param tempdir path to a writable directory for temporary files
	 */
	CModInstaller(const OsPath& modsdir, const OsPath& tempdir);

	~CModInstaller();

	/**
	 * Process and unpack the mod.
	 * @param mod path of .pyromod/.zip file
	 * @param keepFile if true, copy the file, if false move it
	 */
	ModInstallationResult Install(
		const OsPath& mod,
		const std::shared_ptr<ScriptRuntime>& scriptRuntime,
		bool keepFile);

	/**
	 * @return a list of all mods installed so far by this CModInstaller.
	 */
	const std::vector<CStr>& GetInstalledMods() const;

	/**
	 * @return whether the path has a mod-like extension.
	 */
	static bool IsDefaultModExtension(const Path& ext)
	{
		return ext == ".pyromod" || ext == ".zip";
	}

private:
	PIVFS m_VFS;
	OsPath m_ModsDir;
	OsPath m_TempDir;
	VfsPath m_CacheDir;
	std::vector<CStr> m_InstalledMods;
};

#endif // INCLUDED_MODINSTALLER
