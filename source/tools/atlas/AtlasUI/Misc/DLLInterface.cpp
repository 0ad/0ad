#include "stdafx.h"

#include "DLLInterface.h"

#include "General/Datafile.h"
#include "ActorEditor/ActorEditor.h"
#include "ColourTester/ColourTester.h"
#include "ScenarioEditor/ScenarioEditor.h"
#include "FileConverter/FileConverter.h"
#include "ArchiveViewer/ArchiveViewer.h"

#include "GameInterface/MessagePasser.h"

#include "wx/config.h"
#include "wx/debugrpt.h"

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

using namespace AtlasMessage;

MessagePasser<mCommand>* AtlasMessage::g_MessagePasser_Command = NULL;
MessagePasser<mInput>*   AtlasMessage::g_MessagePasser_Input   = NULL;

ATLASDLLIMPEXP void Atlas_SetMessagePasser(MessagePasser<mCommand>* handler_cmd, MessagePasser<mInput>* handler_in)
{
	g_MessagePasser_Command = handler_cmd;
	g_MessagePasser_Input   = handler_in;
}

ATLASDLLIMPEXP void Atlas_StartWindow(wchar_t* type)
{
	g_InitialWindowType = type;
	wxEntry(g_Module);
}


class wxDLLApp : public wxApp
{
public:
	virtual bool OnInit()
	{
		if (! wxIsDebuggerRunning())
			wxHandleFatalExceptions();

		// Initialise the global config file
		wxConfigBase::Set(new wxConfig(_T("Atlas Editor"), _T("Wildfire Games")));

		// Assume that the .exe is located in .../binaries/system. (We can't
		// just use the cwd, since that isn't correct when being executed by
		// dragging-and-dropping onto the program in Explorer.)
		Datafile::SetSystemDirectory(argv[0]);

		// Display the appropriate window
		wxFrame* frame;
#define MAYBE(t) if (g_InitialWindowType == _T(#t)) frame = new t(NULL); else
		MAYBE(ActorEditor)
		MAYBE(ColourTester)
		MAYBE(ScenarioEditor)
		MAYBE(FileConverter)
		MAYBE(ArchiveViewer)
#undef MAYBE
		// else
		{
			wxFAIL_MSG(_("Internal error: invalid window type"));
			return false;
		}

		frame->Show();
		SetTopWindow(frame);

		AtlasWindow* win = wxDynamicCast(frame, AtlasWindow);
		if (win)
		{
			// One argument => argv[1] is probably a filename to open
			if (argc > 1)
			{
				wxChar* filename = argv[1];

				if (filename[0] != _T('-')) // ignore -options
				{
					if (wxFile::Exists(filename))
					{
						win->OpenFile(filename);
					}
					else
						wxLogError(_("Cannot find file '%s'"), filename);
				}
			}
		}

		return true;
	}


	bool OpenDirectory(const wxString& dir)
	{
		// Code largely copied from wxLaunchDefaultBrowser:
		// (TODO: portability)

		typedef HINSTANCE (WINAPI *LPShellExecute)(HWND hwnd, const wxChar* lpOperation,
			const wxChar* lpFile,
			const wxChar* lpParameters,
			const wxChar* lpDirectory,
			INT nShowCmd);

		HINSTANCE hShellDll = ::LoadLibrary(_T("shell32.dll"));
		if (hShellDll == NULL)
			return false;

		LPShellExecute lpShellExecute =
			(LPShellExecute) ::GetProcAddress(hShellDll,
			wxString(_T("ShellExecute")
#ifdef _UNICODE
			  _T("W")
#else
			  _T("A")
#endif
			).mb_str(wxConvLocal));

		if (lpShellExecute == NULL)
			return false;

		/*HINSTANCE nResult =*/ (*lpShellExecute)(NULL, _T("explore"), dir.c_str(), NULL, NULL, SW_SHOWNORMAL);
			// ignore return value, since we're not going to do anything if this fails

		::FreeLibrary(hShellDll);

		return true;
	}

	virtual void OnFatalException()
	{
		wxDebugReport report;
		wxDebugReportPreviewStd preview;

		report.AddAll();

		if (preview.Show(report))
		{
			wxString dir = report.GetDirectory(); // save the string, since it gets cleared by Process
			report.Process();
			OpenDirectory(dir);
		}
	}
};

IMPLEMENT_APP_NO_MAIN(wxDLLApp) 
