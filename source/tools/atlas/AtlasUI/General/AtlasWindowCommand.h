#ifndef ATLASWINDOWCOMMAND_H__
#define ATLASWINDOWCOMMAND_H__

#include "wx/cmdproc.h"

class AtlasWindowCommand : public wxCommand
{
	DECLARE_ABSTRACT_CLASS(AtlasWindowCommand);

	friend class AtlasWindowCommandProc;

public:
	AtlasWindowCommand(bool canUndoIt, const wxString& name)
		: wxCommand(canUndoIt, name), m_Finalized(false) {}

private:
	// Control merging of this command with a future one (so they
	// can be undone in a single step). The function should try to merge
	// with the previous command, by altering that previous command and then
	// returning true. If it can't, return false.
	virtual bool Merge(AtlasWindowCommand* WXUNUSED(previousCommand)) { return false; }

	bool m_Finalized;
};

#endif // ATLASWINDOWCOMMANDPROC_H__
