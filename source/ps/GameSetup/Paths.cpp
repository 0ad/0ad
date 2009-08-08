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

#if OS_WIN
#include "lib/sysdep/os/win/wutil.h"	// win_appdata_dir
#endif
#include "lib/sysdep/sysdep.h"	// sys_get_executable_name


Paths::Paths(const CmdLineArgs& args)
{
	m_root = Root(args.GetArg0());
	m_rdata = m_root/"data";
	const char* subdirectoryName = args.Has("writableRoot")? 0 : "0ad";

	// everything is a subdirectory of the root
	if(!subdirectoryName)
	{
		m_data = m_rdata;
		m_config = m_data/"config";
		m_cache = m_data/"cache";
		m_logs = m_root/"logs";
	}
	else
	{
#if OS_WIN
		const fs::path appdata(fs::path(win_appdata_dir)/subdirectoryName);
		m_data = appdata/"data";
		m_config = appdata/"config";
		m_cache = appdata/"cache";
		m_logs = appdata/"logs";
#else
		const char* envHome = getenv("HOME");
		debug_assert(envHome);
		const fs::path home(envHome);
		m_data = XDG_Path("XDG_DATA_HOME", home, home/".local/share")/subdirectoryName;
		m_config = XDG_Path("XDG_CONFIG_HOME", home, home/".config")/subdirectoryName;
		m_cache = XDG_Path("XDG_CACHE_HOME", home, home/".cache")/subdirectoryName;
		m_logs = m_config/"logs";
#endif
	}
}


/*static*/ fs::path Paths::Root(const CStr& argv0)
{
	// get full path to executable
	char pathname[PATH_MAX];
	// .. first try safe, but system-dependent version
	if(sys_get_executable_name(pathname, PATH_MAX) < 0)
	{
		// .. failed; use argv[0]
		errno = 0;
		if(!realpath(argv0.c_str(), pathname))
			WARN_ERR(LibError_from_errno(false));
	}

	// make sure it's valid
	errno = 0;
	if(access(pathname, X_OK) < 0)
		WARN_ERR(LibError_from_errno(false));

	fs::path path(pathname);
	for(size_t i = 0; i < 3; i++)	// remove "system/name.exe"
		path.remove_leaf();
	return path;
}


/*static*/ fs::path Paths::XDG_Path(const char* envname, const fs::path& home, const fs::path& defaultPath)
{
	const char* path = getenv(envname);
	if(path)
	{
		if(path[0] != '/')	// relative to $HOME
			return home/path;
		return fs::path(path);
	}
	return defaultPath;
}
