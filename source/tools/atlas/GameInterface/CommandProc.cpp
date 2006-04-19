#include "precompiled.h"

#include "CommandProc.h"

#include <algorithm>

//////////////////////////////////////////////////////////////////////////

template<typename T> T next(T x) { T t = x; return ++t; }

template<typename T, typename I> void delete_erase(T list, I first, I last)
{
	while (first != last)
	{
		delete *first;
		first = list.erase(first);
	}
}

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
	debug_assert(!m_Commands.size());
}

void CommandProc::Destroy()
{
	std::for_each(m_Commands.begin(), m_Commands.end(), delete_fn<Command>);
	m_Commands.clear();
}

void CommandProc::Submit(Command* cmd)
{
	delete_erase(m_Commands, next(m_CurrentCommand), m_Commands.end());
	m_CurrentCommand = m_Commands.insert(next(m_CurrentCommand), cmd);

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
	if (next(m_CurrentCommand) != m_Commands.end())
	{
		++m_CurrentCommand;
		(*m_CurrentCommand)->Redo();
	}
}

void CommandProc::Merge()
{
	if (m_CurrentCommand == m_Commands.begin())
	{
		debug_warn("Merge illogic: no commands");
		return;
	}

	if (next(m_CurrentCommand) != m_Commands.end())
	{
		debug_warn("Merge illogic: not at stack top");
		return;
	}

	cmdIt prev = m_CurrentCommand;
	--prev;

	if (prev == m_Commands.begin())
	{
		debug_warn("Merge illogic: only 1 command");
		return;
	}

	if ((*prev)->GetType() != (*m_CurrentCommand)->GetType())
	{
		const char* a = (*prev)->GetType();
		const char* b = (*m_CurrentCommand)->GetType();
		debug_printf("[incompatible: %s -> %s]\n", a, b);
		debug_warn("Merge illogic: incompatible command");
		return;
	}

	(*m_CurrentCommand)->Merge(*prev);

	delete *m_CurrentCommand;
	m_Commands.erase(m_CurrentCommand);

	m_CurrentCommand = prev;
}
