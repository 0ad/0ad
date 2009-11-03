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

	const fs::wpath& Root() const
	{
		return m_root;
	}

	const fs::wpath& RData() const
	{
		return m_rdata;
	}

	const fs::wpath& Data() const
	{
		return m_data;
	}

	const fs::wpath& Config() const
	{
		return m_config;
	}

	const fs::wpath& Cache() const
	{
		return m_cache;
	}

	const fs::wpath& Logs() const
	{
		return m_logs;
	}

private:
	static fs::wpath Root(const CStr& argv0);
	static fs::wpath XDG_Path(const char* envname, const fs::wpath& home, const fs::wpath& defaultPath);

	// read-only directories, fixed paths relative to executable
	fs::wpath m_root;
	fs::wpath m_rdata;

	// writable directories
	fs::wpath m_data;
	fs::wpath m_config;
	fs::wpath m_cache;
	fs::wpath m_logs;	// special-cased in single-root-folder installations
};

#endif	// #ifndef INCLUDED_PS_GAMESETUP_PATHS
