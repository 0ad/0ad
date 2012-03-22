/* Copyright (C) 2012 Wildfire Games.
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

#include "lib/os_path.h"
#include "CmdLineArgs.h"

/**
 * Wrapper class for OS paths used by the game
 */
class Paths
{
public:
	Paths(const CmdLineArgs& args);

	/**
	 * Returns the game's root directory
	 */
	const OsPath& Root() const
	{
		return m_root;
	}

	/**
	 * Returns directory for read-only data installed with the game
	 */
	const OsPath& RData() const
	{
		return m_rdata;
	}

	/**
	 * Returns directory for game-managed data and mods
	 */
	const OsPath& GameData() const
	{
		return m_gameData;
	}

	/**
	 * Returns directory for user-created data
	 * Only things created in response to an explicit user action should go here.
	 * (note: only Windows currently treats this differently than GameData)
	 */
	const OsPath& UserData() const
	{
		return m_userData;
	}

	/**
	 * Returns config file directory
	 */
	const OsPath& Config() const
	{
		return m_config;
	}

	/**
	 * Returns cache directory
	 */
	const OsPath& Cache() const
	{
		return m_cache;
	}

	/**
	 * Returns logs directory
	 */
	const OsPath& Logs() const
	{
		return m_logs;
	}

private:
	static OsPath Root(const OsPath& argv0);
	static OsPath RootData(const OsPath& argv0);
	static OsPath XDG_Path(const char* envname, const OsPath& home, const OsPath& defaultPath);

	// read-only directories, fixed paths relative to executable
	OsPath m_root;
	OsPath m_rdata;

	// writable directories
	OsPath m_gameData;
	OsPath m_userData;
	OsPath m_config;
	OsPath m_cache;
	OsPath m_logs;	// special-cased in single-root-folder installations
};

#endif	// #ifndef INCLUDED_PS_GAMESETUP_PATHS
