#ifndef _COMMANDMANAGER_H
#define _COMMANDMANAGER_H

class CCommand;
#include <list>
#include "res/res.h"

class CCommandManager
{
public:
	CCommandManager();
	~CCommandManager();

	void Append(CCommand* cmd);
	void Execute(CCommand* cmd);
	
	void Undo();
	const char* GetUndoName();
	
	void Redo();
	const char* GetRedoName();

	void ClearHistory();

private:
	typedef std::list<CCommand*> CommandList;

	CommandList::iterator GetIterator(int pos);
	CCommand* GetUndoCommand();
	CCommand* GetRedoCommand();

	i32 m_HistoryPos;
	u32 m_CommandHistoryLength;
	CommandList m_CommandHistory;
};

extern CCommandManager g_CmdMan;

#endif
