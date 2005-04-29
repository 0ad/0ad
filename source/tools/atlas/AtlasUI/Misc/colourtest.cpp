// Just another boring app that creates a ColourTester window

#include "stdafx.h"

#include "ColourTester/ColourTester.h"
#include "Datafile.h"

#include "wx/file.h"
#include "wx/config.h"

#include "wx/generic/colrdlgg.h"
#include "wx/generic/filedlgg.h"
#include "wx/generic/dirctrlg.h"
#include "wx/generic/dirdlgg.h"

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

		// Display the main program window
		wxFrame *frame = new ColourTester(NULL);
		frame->Show();
		SetTopWindow(frame);

/*
		// One argument => argv[1] is a filename to open
		if (argc > 1)
		{
			wxChar* filename = argv[1];
			if (wxFile::Exists(filename))
				frame->OpenFile(filename);
			else
				wxLogError(_("Cannot find file '%s'"), filename);
		}
*/

		return true;
	}

};

IMPLEMENT_APP(MyApp)
