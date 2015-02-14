/* Copyright (C) 2010 Wildfire Games.
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

#include "CommandProc.h"

#include <cassert>
#include <algorithm>

//////////////////////////////////////////////////////////////////////////

template<typename T> T next_it(T x) { T t = x; return ++t; }

template<typename T> void delete_fn(T* v) { delete v; }

//////////////////////////////////////////////////////////////////////////

using namespace AtlasMessage;

namespace AtlasMessage {

	CommandProc& GetCommandProc()
	{
		static CommandProc commandProc;
		return commandProc;
	}

	cmdHandlers& GetCmdHandlers()
	{
		static cmdHandlers h;
		return h;
	}
}


CommandProc::CommandProc()
{
	// Start the list with a NULL, so m_CurrentCommand can point at
	// something even when the command stack is empty
	m_Commands.push_back(NULL);

	m_CurrentCommand = m_Commands.begin();
}

CommandProc::~CommandProc()
{
	// Make sure Destroy has been called before now (to avoid
	// problems from the destruction order of static variables)
	ENSURE(!m_Commands.size());
}

void CommandProc::Destroy()
{
	std::for_each(m_Commands.begin(), m_Commands.end(), delete_fn<Command>);
	m_Commands.clear();
}

void CommandProc::Submit(Command* cmd)
{
	// If some commands have been undone at the time we insert this new one,
	// delete and remove them all.
	std::for_each(next_it(m_CurrentCommand), m_Commands.end(), delete_fn<Command>);
	m_Commands.erase(next_it(m_CurrentCommand), m_Commands.end());
	assert(next_it(m_CurrentCommand) == m_Commands.end());

	m_CurrentCommand = m_Commands.insert(next_it(m_CurrentCommand), cmd);

	(*m_CurrentCommand)->Do();
}

void CommandProc::Undo()
{
	if (m_CurrentCommand != m_Commands.begin())
	{
		(*m_CurrentCommand)->Undo();
		--m_CurrentCommand;
	}
}

void CommandProc::Redo()
{
	if (next_it(m_CurrentCommand) != m_Commands.end())
	{
		++m_CurrentCommand;
		(*m_CurrentCommand)->Redo();
	}
}

void CommandProc::Merge()
{
	if (m_CurrentCommand == m_Commands.begin())
	{
		debug_warn(L"Merge illogic: no commands");
		return;
	}

	if (next_it(m_CurrentCommand) != m_Commands.end())
	{
		debug_warn(L"Merge illogic: not at stack top");
		return;
	}

	cmdIt prev = m_CurrentCommand;
	--prev;

	if (prev == m_Commands.begin())
	{
		debug_warn(L"Merge illogic: only 1 command");
		return;
	}

	if ((*prev)->GetType() != (*m_CurrentCommand)->GetType())
	{
		const char* a = (*prev)->GetType();
		const char* b = (*m_CurrentCommand)->GetType();
		debug_printf("[incompatible: %s -> %s]\n", a, b);
		debug_warn(L"Merge illogic: incompatible command");
		return;
	}

	(*m_CurrentCommand)->Merge(*prev);

	delete *m_CurrentCommand;
	m_Commands.erase(m_CurrentCommand);

	m_CurrentCommand = prev;
}
