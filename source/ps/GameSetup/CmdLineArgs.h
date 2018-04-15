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

#ifndef INCLUDED_CMDLINEARGS
#define INCLUDED_CMDLINEARGS

#include "ps/CStr.h"
#include "lib/os_path.h"

class CmdLineArgs
{
public:
	CmdLineArgs() {}

	/**
	 * Parse the command-line options, for future processing.
	 * All arguments are required to be of the form <tt>-name</tt> or
	 * <tt>-name=value</tt> - anything else is ignored.
	 *
	 * @param argc size of argv array
	 * @param argv array of arguments; argv[0] should be the program's name
	 */
	CmdLineArgs(int argc, const char* argv[]);

	/**
	 * Test whether the given name was specified, as either <tt>-name</tt> or
	 * <tt>-name=value</tt>
	 */
	bool Has(const CStr& name) const;

	/**
	 * Get the value of the named parameter. If it was not specified, returns
	 * the empty string. If it was specified multiple times, returns the value
	 * from the first occurrence.
	 */
	CStr Get(const CStr& name) const;

	/**
	 * Get all the values given to the named parameter. Returns values in the
	 * same order as they were given in argv.
	 */
	std::vector<CStr> GetMultiple(const CStr& name) const;

	/**
	 * Get the value of argv[0], which is typically meant to be the name/path of
	 * the program (but the actual value is up to whoever executed the program).
	 */
	OsPath GetArg0() const;

	/**
	 * Returns all arguments that don't have a name (string started with '-').
	 */
	std::vector<CStr> GetArgsWithoutName() const;

private:
	typedef std::vector<std::pair<CStr, CStr> > ArgsT;
	ArgsT m_Args;
	OsPath m_Arg0;
	std::vector<CStr> m_ArgsWithoutName;
};

#endif // INCLUDED_CMDLINEARGS
