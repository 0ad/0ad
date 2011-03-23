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
#include "Paths.h"

#include "lib/file/file_system_util.h"
#include "lib/sysdep/sysdep.h"	// sys_get_executable_name
#include "lib/sysdep/filesystem.h"	// wrealpath
#if OS_WIN
# include "lib/sysdep/os/win/wutil.h"	// wutil_AppdataPath
#endif


Paths::Paths(const CmdLineArgs& args)
{
	m_root = Root(args.GetArg0());

#ifdef INSTALLED_DATADIR
	m_rdata = WIDEN(STRINGIZE(INSTALLED_DATADIR)) L"/";
#else
	m_rdata = m_root/"data/";
#endif

	const char* subdirectoryName = args.Has("writableRoot")? 0 : "0ad";

	// everything is a subdirectory of the root
	if(!subdirectoryName)
	{
		m_data = m_rdata;
		m_config = m_data/"config/";
		m_cache = m_data/"cache/";
		m_logs = m_root/"logs/";
	}
	else
	{
#if OS_WIN
		const OsPath appdata = wutil_AppdataPath() / subdirectoryName/"";
		m_data = appdata/"data/";
		m_config = appdata/"config/";
		m_cache = appdata/"cache/";
		m_logs = appdata/"logs/";
#else
		const char* envHome = getenv("HOME");
		debug_assert(envHome);
		const OsPath home(envHome);
		const OsPath xdgData   = XDG_Path("XDG_DATA_HOME",   home, home/".local/share/") / subdirectoryName;
		const OsPath xdgConfig = XDG_Path("XDG_CONFIG_HOME", home, home/".config/"     ) / subdirectoryName;
		const OsPath xdgCache  = XDG_Path("XDG_CACHE_HOME",  home, home/".cache/"      ) / subdirectoryName;
		m_data   = xdgData/"";
		m_cache  = xdgCache/"";
		m_config = xdgConfig/"config/";
		m_logs   = xdgConfig/"logs/";
#endif
	}
}


/*static*/ OsPath Paths::Root(const OsPath& argv0)
{
	// get full path to executable
	OsPath pathname;
	// .. first try safe, but system-dependent version
	if(sys_get_executable_name(pathname) != INFO::OK)
	{
		// .. failed; use argv[0]
		errno = 0;
		pathname = wrealpath(argv0);
		if(pathname.empty())
			WARN_ERR(LibError_from_errno(false));
	}

	// make sure it's valid
	if(!fs_util::FileExists(pathname))
		WARN_ERR(LibError_from_errno(false));

	fs::wpath components = pathname.string();
	for(size_t i = 0; i < 3; i++)	// remove "system/name.exe"
		components.remove_leaf();
	return components.string();
}


/*static*/ OsPath Paths::XDG_Path(const char* envname, const OsPath& home, const OsPath& defaultPath)
{
	const char* path = getenv(envname);
	if(path)
	{
		if(path[0] != '/')	// relative to $HOME
			return home / path/"";
		return OsPath(path)/"";
	}
	return defaultPath/"";
}
