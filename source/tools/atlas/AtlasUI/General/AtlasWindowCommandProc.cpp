#include "stdafx.h"

#include "AtlasWindow.h"
#include "AtlasWindowCommand.h"
#include "AtlasDialog.h"

AtlasWindowCommandProc* AtlasWindowCommandProc::GetFromParentFrame(wxWindow* object)
{
	wxWindow* win = object;
	while (win)
	{
		{
			AtlasWindow* tgt = wxDynamicCast(win, AtlasWindow);
			if (tgt)
				return &tgt->m_CommandProc;
		}
		{
			AtlasDialog* tgt = wxDynamicCast(win, AtlasDialog);
			if (tgt)
				return &tgt->m_CommandProc;
		}

		win = win->GetParent();
	}
	wxASSERT_MSG(0, _T("Couldn't find command processor"));
	return NULL;
}

bool AtlasWindowCommandProc::Submit(wxCommand *command, bool storeIt)
{
	wxCHECK_MSG(command, false, _T("no command in wxCommandProcessor::Submit"));

	AtlasWindowCommand* previousCommand = wxDynamicCast(GetCurrentCommand(), AtlasWindowCommand);

	if (! DoCommand(*command))
	{
		delete command;

		return false;
	}

	if (storeIt)
	{
		AtlasWindowCommand* currentCommand = wxDynamicCast(command, AtlasWindowCommand);
		
		if (currentCommand && previousCommand
			&& !previousCommand->m_Finalized
			&& previousCommand->Merge(currentCommand))
		{
			delete command;
		}
		else
			Store(command);
	}
	else
		delete command;

	return true;
}

void AtlasWindowCommandProc::FinaliseLastCommand()
{
	AtlasWindowCommand* previousCommand = wxDynamicCast(GetCurrentCommand(), AtlasWindowCommand);
	if (previousCommand)
		previousCommand->m_Finalized = true;
}
