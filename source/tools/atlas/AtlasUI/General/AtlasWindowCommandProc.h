#ifndef ATLASWINDOWCOMMANDPROC_H__
#define ATLASWINDOWCOMMANDPROC_H__

#include "wx/cmdproc.h"

class AtlasWindowCommandProc : public wxCommandProcessor
{
public:
	bool Submit(wxCommand *command, bool storeIt = true);

	// Mark the most recent command as finalized, so it won't be
	// merged with any subsequent ones
	void FinaliseLastCommand();

	static AtlasWindowCommandProc* GetFromParentFrame(wxWindow* object);
};

#endif // ATLASWINDOWCOMMANDPROC_H__
