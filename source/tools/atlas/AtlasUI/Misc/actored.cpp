// Just a boring app that creates an ActorEditor window

#include "stdafx.h"

#include "ActorEditor/ActorEditor.h"
#include "Datafile.h"

#include "wx/file.h"
#include "wx/config.h"

class MyApp: public wxApp
{
	bool OnInit()
	{
		// Initialise the global config file
		wxConfigBase::Set(new wxConfig(_T("Atlas Editor"), _T("Wildfire Games")));

		// Assume that the .exe is located in .../binaries/system. (We can't
		// just use the cwd, since that isn't correct when being executed by
		// dragging-and-dropping onto the program in Explorer.)
		Datafile::SetSystemDirectory(argv[0]);

		// Display the Actor Editor window
		AtlasWindow *frame = new ActorEditor(NULL);
		frame->Show();
		SetTopWindow(frame);

		// One argument => argv[1] is a filename to open
		if (argc > 1)
		{
			wxChar* filename = argv[1];
			if (wxFile::Exists(filename))
				frame->OpenFile(filename);
			else
				wxLogError(_("Cannot find file '%s'"), filename);
		}

		return true;
	}

};

IMPLEMENT_APP(MyApp)
