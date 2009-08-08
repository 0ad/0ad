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

#ifndef INCLUDED_PS_GAMESETUP_PATHS
#define INCLUDED_PS_GAMESETUP_PATHS

#include "CmdLineArgs.h"

class Paths
{
public:
	Paths(const CmdLineArgs& args);

	const fs::path& Root() const
	{
		return m_root;
	}

	const fs::path& RData() const
	{
		return m_rdata;
	}

	const fs::path& Data() const
	{
		return m_data;
	}

	const fs::path& Config() const
	{
		return m_config;
	}

	const fs::path& Cache() const
	{
		return m_cache;
	}

	const fs::path& Logs() const
	{
		return m_logs;
	}

private:
	static fs::path Root(const CStr& argv0);
	static fs::path XDG_Path(const char* envname, const fs::path& home, const fs::path& defaultPath);

	// read-only directories, fixed paths relative to executable
	fs::path m_root;
	fs::path m_rdata;

	// writable directories
	fs::path m_data;
	fs::path m_config;
	fs::path m_cache;
	fs::path m_logs;	// special-cased in single-root-folder installations
};

#endif	// #ifndef INCLUDED_PS_GAMESETUP_PATHS
