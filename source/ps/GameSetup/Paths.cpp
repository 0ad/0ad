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

#include "lib/path_util.h"
#include "lib/utf8.h"
#include "lib/sysdep/filesystem.h"	// wrealpath
#include "lib/sysdep/sysdep.h"	// sys_get_executable_name
#if OS_WIN
# include "lib/sysdep/os/win/wutil.h"	// wutil_AppdataPath
#endif


Paths::Paths(const CmdLineArgs& args)
{
	m_root = Root(wstring_from_utf8(args.GetArg0()));

#ifdef INSTALLED_DATADIR
	m_rdata = WIDEN(STRINGIZE(INSTALLED_DATADIR)) L"/";
#else
	m_rdata = Path::Join(m_root, L"data/");
#endif

	const wchar_t* subdirectoryName = args.Has("writableRoot")? 0 : L"0ad";

	// everything is a subdirectory of the root
	if(!subdirectoryName)
	{
		m_data = m_rdata;
		m_config = Path::Join(m_data, L"config/");
		m_cache = Path::Join(m_data, L"cache/");
		m_logs = Path::Join(m_root, L"logs/");
	}
	else
	{
#if OS_WIN
		const NativePath appdata = Path::AddSlash(Path::Join(wutil_AppdataPath(), subdirectoryName));
		m_data = Path::Join(appdata, L"data/");
		m_config = Path::Join(appdata, L"config/");
		m_cache = Path::Join(appdata, L"cache/");
		m_logs = Path::Join(appdata, L"logs/");
#else
		const char* envHome = getenv("HOME");
		debug_assert(envHome);
		const NativePath home(wstring_from_utf8(envHome));
		const NativePath xdgData = XDG_Path("XDG_DATA_HOME", home, Path::Join(home, L".local/share/", subdirectoryName));
		const NativePath xdgConfig = XDG_Path("XDG_CONFIG_HOME", home, Path::Join(home, L".config/", subdirectoryName));
		const NativePath xdgCache = XDG_Path("XDG_CACHE_HOME", home, Path::Join(home, L".cache/", subdirectoryName));
		m_data = Path::AddSlash(xdgData);
		m_cache = Path::AddSlash(xdgCache);
		m_config = Path::AddSlash(Path::Join(xdgConfig, L"config"));
		m_logs = Path::AddSlash(Path::Join(xdgConfig, L"logs"));
#endif
	}
}


/*static*/ NativePath Paths::Root(const std::wstring& argv0)
{
	// get full path to executable
	NativePath pathname;
	// .. first try safe, but system-dependent version
	if(sys_get_executable_name(pathname) != INFO::OK)
	{
		// .. failed; use argv[0]
		wchar_t pathname_buf[PATH_MAX];
		errno = 0;
		if(!wrealpath(argv0.c_str(), pathname_buf))
			WARN_ERR(LibError_from_errno(false));
		pathname = pathname_buf;
	}

	// make sure it's valid
	if(!FileExists(pathname))
		WARN_ERR(LibError_from_errno(false));

	fs::wpath components = pathname;
	for(size_t i = 0; i < 3; i++)	// remove "system/name.exe"
		components.remove_leaf();
	return components.string();
}


/*static*/ NativePath Paths::XDG_Path(const char* envname, const NativePath& home, const NativePath& defaultPath)
{
	const char* path = getenv(envname);
	if(path)
	{
		if(path[0] != '/')	// relative to $HOME
			return Path::AddSlash(Path::Join(home, wstring_from_utf8(path)));
		return Path::AddSlash(wstring_from_utf8(path));
	}
	return Path::AddSlash(defaultPath);
}
