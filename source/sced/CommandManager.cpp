#include "Command.h"
#include "CommandManager.h"

CCommandManager g_CmdMan;

CCommandManager::CCommandManager() : m_CommandHistoryLength(128), m_HistoryPos(-1)
{
}

CCommandManager::~CCommandManager()
{
	ClearHistory();
}

/////////////////////////////////////////////////////////////////////////////////////////
// Append: add given command to the end of the history list, without
// executing it
void CCommandManager::Append(CCommand* cmd)
{
	if (cmd->IsUndoable()) {
		// remove all existing commands from current position to end
		CommandList::iterator iter=GetIterator(m_HistoryPos+1);
		for (CommandList::iterator i=iter;i!=m_CommandHistory.end();++i) {
			delete *i;
		}
		m_CommandHistory.erase(iter,m_CommandHistory.end());

		// add new command to end
		m_CommandHistory.push_back(cmd);

		// check for history that's too big
		if (m_CommandHistory.size()>m_CommandHistoryLength) {
			CCommand* cmd=m_CommandHistory.front();
			m_CommandHistory.pop_front();
			delete cmd;
		}

		// update history position to point at last command executed
		m_HistoryPos=m_CommandHistory.size()-1;
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
// Execute: execute the given command, then add it to the end of the history list
void CCommandManager::Execute(CCommand* cmd)
{
	cmd->Execute();
	Append(cmd);
}
	
CCommandManager::CommandList::iterator CCommandManager::GetIterator(int pos)
{
	if (pos<0 || uint(pos)>=m_CommandHistory.size()) return m_CommandHistory.end();

	typedef CommandList::iterator Iter;
	int count=0;
	for (Iter iter=m_CommandHistory.begin();iter!=m_CommandHistory.end();++iter) {
		if (count==pos) {
			return iter;
		}
		count++;
	}

	// hmm .. shouldn't get here
	return m_CommandHistory.end();
}

CCommand* CCommandManager::GetUndoCommand()
{
	CommandList::iterator iter=GetIterator(m_HistoryPos);
	if (iter!=m_CommandHistory.end()) {
		return *iter;
	} else {
		return 0;
	}
}

void CCommandManager::Undo()
{
	CCommand* cmd=GetUndoCommand();
	if (cmd) {
		cmd->Undo();
		m_HistoryPos--;
	}
}

const char* CCommandManager::GetUndoName()
{
	CCommand* cmd=GetUndoCommand();
	return cmd ? cmd->GetName() : 0;
}
	
CCommand* CCommandManager::GetRedoCommand()
{
	CommandList::iterator iter=GetIterator(m_HistoryPos+1);
	if (iter!=m_CommandHistory.end()) {
		return *iter;
	} else {
		return 0;
	}
}

void CCommandManager::Redo()
{
	CCommand* cmd=GetRedoCommand();
	if (cmd) {
		cmd->Redo();
		m_HistoryPos++;
	}
}

const char* CCommandManager::GetRedoName()
{
	CCommand* cmd=GetRedoCommand();
	return cmd ? cmd->GetName() : 0;
}


void CCommandManager::ClearHistory()
{
	// empty out command history
	while (m_CommandHistory.size()>0) {
		CCommand* cmd=m_CommandHistory.front();
		m_CommandHistory.pop_front();
		delete cmd;
	}		
}
