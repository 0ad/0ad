#include "stdafx.h"

#include "DLLInterface.h"

#include "Datafile.h"
#include "ActorEditor/ActorEditor.h"
#include "ColourTester/ColourTester.h"

#include "wx/config.h"

// Global variables, to remember state between DllMain and StartWindow and OnInit
wxString g_InitialWindowType;
HINSTANCE g_Module;
bool g_IsLoaded = false;


BOOL APIENTRY DllMain(HINSTANCE hModule, DWORD fdwReason, LPVOID WXUNUSED(lpReserved))
{
	switch (fdwReason)
	{
	case DLL_PROCESS_ATTACH:
		g_Module = hModule;
		return TRUE;

	case DLL_PROCESS_DETACH:
		if (g_IsLoaded)
		{
			wxEntryCleanup();
			g_IsLoaded = false;
		}
		break;
	}

	return TRUE;
}


ATLASDLLIMPEXP void StartWindow(wchar_t* type)
{
	g_InitialWindowType = type;
	wxEntry(g_Module);
}


class wxDLLApp : public wxApp
{
public:
	virtual bool OnInit()
	{
		// Initialise the global config file
		wxConfigBase::Set(new wxConfig(_T("Atlas Editor"), _T("Wildfire Games")));

		// Assume that the .exe is located in .../binaries/system. (We can't
		// just use the cwd, since that isn't correct when being executed by
		// dragging-and-dropping onto the program in Explorer.)
		Datafile::SetSystemDirectory(argv[0]);

		// Display the Actor Editor window
		wxFrame* frame;
		if (g_InitialWindowType == _T("ActorEditor"))
			frame = new ActorEditor(NULL);
		else if (g_InitialWindowType == _T("ColourTester"))
			frame = new ColourTester(NULL);
		else
		{
			wxFAIL_MSG(_("Internal error: invalid window type"));
			return false;
		}

		frame->Show();
		SetTopWindow(frame);

		// One argument => argv[1] is a filename to open
		if (argc > 1)
		{
			wxChar* filename = argv[1];
			if (wxFile::Exists(filename))
			{
				AtlasWindow* win = wxDynamicCast(frame, AtlasWindow);
				if (win)
					win->OpenFile(filename);
			}
			else
				wxLogError(_("Cannot find file '%s'"), filename);
		}

		return true;
	}
};

IMPLEMENT_APP_NO_MAIN(wxDLLApp) 
