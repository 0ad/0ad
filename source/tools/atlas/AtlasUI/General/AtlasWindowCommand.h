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
	// can be undone in a single step)
	virtual bool Merge(AtlasWindowCommand* command)=0;

	bool m_Finalized;
};
