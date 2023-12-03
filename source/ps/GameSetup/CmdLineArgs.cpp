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
#include "CmdLineArgs.h"

#include "lib/sysdep/sysdep.h"

#include <algorithm>
#include <iterator>
#include <string_view>

CmdLineArgs g_CmdLineArgs;

namespace
{

// Simple matcher for elements of the arguments container.
class IsKeyEqualTo
{
public:
	IsKeyEqualTo(const CStr& value) : m_Value(value) {}

	bool operator()(const std::pair<CStr, CStr>& p) const
	{
		return p.first == m_Value;
	}

private:
	const CStr m_Value;
};

} // namespace

CmdLineArgs::CmdLineArgs(const PS::span<const char* const> argv)
{
	if (argv.empty())
		return;

	std::string arg0(argv[0]);
	// avoid OsPath complaining about mixing both types of separators,
	// which happens when running in the VC2010 debugger
	std::replace(arg0.begin(), arg0.end(), '/', SYS_DIR_SEP);
	m_Arg0 = arg0;

	// Implicit conversion from const char* to std::string_view.
	for (const std::string_view arg : argv.subspan(1))
	{
		// Only accept arguments that start with '-'
		if (arg[0] != '-')
		{
			m_ArgsWithoutName.emplace_back(arg);
			continue;
		}

		// Allow -arg and --arg
		const std::string_view afterDashes = arg.substr(arg[1] == '-' ? 2 : 1);

		// Check for "-arg=value"
		const std::string_view::iterator eq =
			std::find(afterDashes.begin(), afterDashes.end(), '=');

		CStr value = (eq != afterDashes.end()) ? CStr{eq + 1, afterDashes.end()} : CStr{};

		m_Args.emplace_back(CStr(afterDashes.begin(), eq), std::move(value));
	}
}

bool CmdLineArgs::Has(const CStr& name) const
{
	return std::any_of(m_Args.begin(), m_Args.end(), IsKeyEqualTo(name));
}

CStr CmdLineArgs::Get(const CStr& name) const
{
	ArgsT::const_iterator it = std::find_if(m_Args.begin(), m_Args.end(), IsKeyEqualTo(name));
	return it != m_Args.end() ? it->second : "";
}

std::vector<CStr> CmdLineArgs::GetMultiple(const CStr& name) const
{
	std::vector<CStr> values;
	ArgsT::const_iterator it = m_Args.begin();
	while ((it = std::find_if(it, m_Args.end(), IsKeyEqualTo(name))) != m_Args.end())
	{
		values.push_back(it->second);
		// Start searching from the next one in the next iteration
		++it;
	}
	return values;
}

OsPath CmdLineArgs::GetArg0() const
{
	return m_Arg0;
}

std::vector<CStr> CmdLineArgs::GetArgsWithoutName() const
{
	return m_ArgsWithoutName;
}
