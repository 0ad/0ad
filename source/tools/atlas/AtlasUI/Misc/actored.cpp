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

		// Display the Actor Editor window
		AtlasWindow *frame = new ActorEditor(NULL);
		frame->Show();
		SetTopWindow(frame);

		// One argument => argv[1] is a filename to open
		if (argc > 1)
		{
			// We were probably executed by dragging a file onto the icon.
			// The working directory is then different to the exe's directory,
			// so we need to set it. (In the normal no-argument case, it should
			// be safe to assume that we're being run from the right directory.)
			Datafile::SetSystemDirectory(argv[0]);

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
