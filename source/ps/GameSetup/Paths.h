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

#include "lib/native_path.h"
#include "CmdLineArgs.h"

class Paths
{
public:
	Paths(const CmdLineArgs& args);

	const NativePath& Root() const
	{
		return m_root;
	}

	const NativePath& RData() const
	{
		return m_rdata;
	}

	const NativePath& Data() const
	{
		return m_data;
	}

	const NativePath& Config() const
	{
		return m_config;
	}

	const NativePath& Cache() const
	{
		return m_cache;
	}

	const NativePath& Logs() const
	{
		return m_logs;
	}

private:
	static NativePath Root(const std::wstring& argv0);
	static NativePath XDG_Path(const char* envname, const NativePath& home, const NativePath& defaultPath);

	// read-only directories, fixed paths relative to executable
	NativePath m_root;
	NativePath m_rdata;

	// writable directories
	NativePath m_data;
	NativePath m_config;
	NativePath m_cache;
	NativePath m_logs;	// special-cased in single-root-folder installations
};

#endif	// #ifndef INCLUDED_PS_GAMESETUP_PATHS
