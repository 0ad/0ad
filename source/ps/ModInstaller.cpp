/* Copyright (C) 2022 Wildfire Games.
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

#include "ModInstaller.h"

#include "lib/file/vfs/vfs_util.h"
#include "lib/file/file_system.h"
#include "lib/sysdep/os.h"
#include "ps/CLogger.h"
#include "ps/Filesystem.h"
#include "ps/XML/Xeromyces.h"
#include "scriptinterface/ScriptInterface.h"
#include "scriptinterface/JSON.h"

#if !OS_WIN
#include "lib/os_path.h"
#endif

#include <fstream>
#if OS_WIN
#include <filesystem>
#endif

CModInstaller::CModInstaller(const OsPath& modsdir, const OsPath& tempdir) :
	m_ModsDir(modsdir), m_TempDir(tempdir / "_modscache"), m_CacheDir("cache/")
{
	m_VFS = CreateVfs();
	CreateDirectories(m_TempDir, 0700);
}

CModInstaller::~CModInstaller()
{
	m_VFS.reset();
	DeleteDirectory(m_TempDir);
}

CModInstaller::ModInstallationResult CModInstaller::Install(
	const OsPath& mod,
	const std::shared_ptr<ScriptContext>& scriptContext,
	bool keepFile)
{
	const OsPath modTemp = m_TempDir / mod.Basename() / mod.Filename().ChangeExtension(L".zip");
	CreateDirectories(modTemp.Parent(), 0700);

	if (keepFile)
	{
		if (CopyFile(mod, modTemp, true) != INFO::OK)
		{
			LOGERROR("Failed to copy '%s' to '%s'", mod.string8().c_str(), modTemp.string8().c_str());
			return FAIL_ON_MOD_COPY;
		}
	}
	else if (RenameFile(mod, modTemp) != INFO::OK)
	{
		LOGERROR("Failed to rename '%s' into '%s'", mod.string8().c_str(), modTemp.string8().c_str());
		return FAIL_ON_MOD_MOVE;
	}

	// Load the mod to VFS
	if (m_VFS->Mount(m_CacheDir, m_TempDir / "") != INFO::OK)
		return FAIL_ON_VFS_MOUNT;
	CVFSFile modinfo;
	PSRETURN modinfo_status = modinfo.Load(m_VFS, m_CacheDir / modTemp.Basename() / "mod.json", false);
	m_VFS->Clear();
	if (modinfo_status != PSRETURN_OK)
		return FAIL_ON_MOD_LOAD;

	// Extract the name of the mod
	CStr modName;
	{
		ScriptInterface scriptInterface("Engine", "ModInstaller", scriptContext);
		ScriptRequest rq(scriptInterface);

		JS::RootedValue json_val(rq.cx);
		if (!Script::ParseJSON(rq, modinfo.GetAsString(), &json_val))
			return FAIL_ON_PARSE_JSON;
		JS::RootedObject json_obj(rq.cx, json_val.toObjectOrNull());
		JS::RootedValue name_val(rq.cx);
		if (!JS_GetProperty(rq.cx, json_obj, "name", &name_val))
			return FAIL_ON_EXTRACT_NAME;
		Script::FromJSVal(rq, name_val, modName);
		if (modName.empty())
			return FAIL_ON_EXTRACT_NAME;
	}

	const OsPath modDir = m_ModsDir / modName;
	const OsPath modPath = modDir / (modName + ".zip");

	// Create a directory with the following structure:
	//   mod-name/
	//     mod-name.zip
	//     mod.json
	CreateDirectories(modDir, 0700);
	if (RenameFile(modTemp, modPath) != INFO::OK)
	{
		LOGERROR("Failed to rename '%s' into '%s'", modTemp.string8().c_str(), modPath.string8().c_str());
		return FAIL_ON_MOD_MOVE;
	}

	DeleteDirectory(modTemp.Parent());

#if OS_WIN
	const std::filesystem::path modJsonPath = (modDir / L"mod.json").fileSystemPath();
#else
	const char* modJsonPath = OsString(modDir / L"mod.json").c_str();
#endif
	std::ofstream mod_json(modJsonPath);
	if (mod_json.good())
	{
		mod_json << modinfo.GetAsString();
		mod_json.close();
	}
	else
		return FAIL_ON_JSON_WRITE;

	m_InstalledMods.emplace_back(modName);

	return SUCCESS;
}

const std::vector<CStr>& CModInstaller::GetInstalledMods() const
{
	return m_InstalledMods;
}
