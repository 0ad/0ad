#ifndef ATLASWINDOWCOMMAND_H__
#define ATLASWINDOWCOMMAND_H__

#include "wx/cmdproc.h"
#include "AtlasObject/AtlasObject.h"
class IAtlasSerialiser;

class AtlasWindowCommand : public wxCommand
{
	DECLARE_ABSTRACT_CLASS(AtlasWindowCommand);

	friend class AtlasWindowCommandProc;

public:
	AtlasWindowCommand(bool canUndoIt, const wxString& name)
		: wxCommand(canUndoIt, name), m_Finalized(false)
	{ }

	// Control merging of this command with a future one (so they
	// can be undone in a single step). Called after 'Do'.
	// The function should try to merge with the previous command,
	// by altering that previous command and then returning true.
	// If it can't, return false.
	virtual bool Merge(AtlasWindowCommand* WXUNUSED(previousCommand)) { return false; }

private:
	bool m_Finalized;
};

//////////////////////////////////////////////////////////////////////////

class AtlasCommand_Begin : public AtlasWindowCommand
{
	DECLARE_CLASS(AtlasCommand_Begin);

	friend class AtlasCommand_End;

public:
	AtlasCommand_Begin(const wxString& description, IAtlasSerialiser* object);

	bool Do();
	bool Undo();

private:
	IAtlasSerialiser* m_Object;
	AtObj m_PreData, m_PostData;
};

class AtlasCommand_End : public AtlasWindowCommand
{
	DECLARE_CLASS(AtlasCommand_End);

public:
	AtlasCommand_End() : AtlasWindowCommand(true, _T("[error]")) {}

	bool Do() { return true; }
	bool Undo() { return false; }
	bool Merge(AtlasWindowCommand* previousCommand);
};


#endif // ATLASWINDOWCOMMANDPROC_H__
