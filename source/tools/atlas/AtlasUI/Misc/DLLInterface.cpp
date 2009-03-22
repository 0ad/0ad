#include "precompiled.h"

#include "DLLInterface.h"

#include "General/AtlasEventLoop.h"

#include "General/Datafile.h"

#include "ActorEditor/ActorEditor.h"
#include "ActorViewer/ActorViewer.h"
#include "ArchiveViewer/ArchiveViewer.h"
#include "ColourTester/ColourTester.h"
#include "FileConverter/FileConverter.h"
#include "ScenarioEditor/ScenarioEditor.h"
#include "ErrorReporter/ErrorReporter.h"

#include "GameInterface/MessagePasser.h"

#include "AtlasScript/ScriptInterface.h"

#include "wx/config.h"
#include "wx/debugrpt.h"
#include "wx/file.h"

#include <libxml/parser.h>

#ifndef LIBXML_THREAD_ENABLED
#error libxml2 must have threading support enabled
#endif

#ifdef __WXGTK__
#include <X11/Xlib.h>
#endif

// Shared memory allocation functions
ATLASDLLIMPEXP void* ShareableMalloc(size_t n)
{
	// TODO: make sure this is thread-safe everywhere. (It is in MSVC with the
	// multithreaded CRT.)
	return malloc(n);
}
ATLASDLLIMPEXP void ShareableFree(void* p)
{
	free(p);
}
// Define the function pointers that we'll use when calling those functions.
// (The game loads the addresses of the above functions, then does the same.)
namespace AtlasMessage
{
	void* (*ShareableMallocFptr) (size_t) = &ShareableMalloc;
	void (*ShareableFreeFptr) (void*) = &ShareableFree;
}


// Global variables, to remember state between DllMain and StartWindow and OnInit
wxString g_InitialWindowType;
bool g_IsLoaded = false;

#ifdef __WXMSW__
HINSTANCE g_Module;

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
#endif // __WXMSW__

using namespace AtlasMessage;

MessagePasser* AtlasMessage::g_MessagePasser = NULL;

ATLASDLLIMPEXP void Atlas_SetMessagePasser(MessagePasser* passer)
{
	g_MessagePasser = passer;
}

ATLASDLLIMPEXP void Atlas_StartWindow(const wchar_t* type)
{
	// Initialise libxml2
	// (If we're executed from the game instead, it has the responsibility to initialise libxml2)
	xmlInitParser();
	
	g_InitialWindowType = type;
#ifdef __WXMSW__
	wxEntry(g_Module);
#else
#ifdef __WXGTK__
	// Because we do GL calls from a secondary thread, Xlib needs to
	// be told to support multiple threads safely
	int status = XInitThreads();
	if (status == 0)
	{
		fprintf(stderr, "Error enabling thread-safety via XInitThreads\n");
	}
#endif
	int argc = 1;
	char atlas[] = "atlas";
	char *argv[] = {atlas, NULL};
	wxEntry(argc, argv);
#endif
}

ATLASDLLIMPEXP void Atlas_DisplayError(const wchar_t* text, size_t WXUNUSED(flags))
{
	// This is called from the game thread.
	// wxLog appears to be thread-safe, so that's okay.
	wxLogError(L"%s", text);

	// TODO: wait for user response (if possible) before returning,
	// and return their status (break/continue/debug/etc), but only in
	// cases where we're certain it won't deadlock (i.e. the UI event loop
	// is still running and won't block before showing the dialog to the user)
	// and where it matters (i.e. errors, not warnings (unless they're going to
	// turn into errors after continuing))

	// TODO: 'text' (or at least some copy of it) appears to get leaked when
	// this function is called
}


ATLASDLLIMPEXP void Atlas_ReportError()
{
	///ReportError();	// janwas: disabled until ErrorReporter.cpp compiles
}


class AtlasDLLApp : public wxApp
{
public:
	AtlasDLLApp()
	: m_ScriptInterface(NULL)
	{
	}
	
	~AtlasDLLApp()
	{
		delete m_ScriptInterface;
	}

	virtual bool OnInit()
	{
// 		_CrtSetBreakAlloc(5632);

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
		MAYBE(ActorViewer)
		MAYBE(ColourTester)
#ifdef USE_AOE3ED
		MAYBE(ArchiveViewer)
		MAYBE(FileConverter)
#endif
#undef MAYBE
		// else
		if (g_InitialWindowType == _T("ScenarioEditor"))
		{
			m_ScriptInterface = new ScriptInterface();
			frame = new ScenarioEditor(NULL, *m_ScriptInterface);
		}
		else
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

/* Disabled (and should be removed if it turns out to be unnecessary)
- see MessagePasserImpl.cpp for information
	virtual int MainLoop()
	{
		// Override the default MainLoop so that we can provide our own event loop

		wxEventLoop* old = m_mainLoop;
		m_mainLoop = new AtlasEventLoop;

		int ret = m_mainLoop->Run();

		delete m_mainLoop;
		m_mainLoop = old;
		return ret;
	}
*/

private:
	ScriptInterface* m_ScriptInterface;

	bool OpenDirectory(const wxString& dir)
	{
		// Open a directory on the filesystem - used so people can find the
		// debug report files generated in OnFatalException easily

#ifdef __WXMSW__
		// Code largely copied from wxLaunchDefaultBrowser:

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
# ifdef _UNICODE
			_T("W")
# else
			_T("A")
# endif
			).mb_str(wxConvLocal));

		if (lpShellExecute == NULL)
			return false;

		/*HINSTANCE nResult =*/ (*lpShellExecute)(NULL, _T("explore"), dir.c_str(), NULL, NULL, SW_SHOWNORMAL);
		// ignore return value, since we're not going to do anything if this fails

		::FreeLibrary(hShellDll);

		return true;
#else
		// Figure out what goes for "default browser" on unix/linux/whatever
		// open an xterm perhaps? :)
		(void)dir;
		return false;
#endif
	}
};

IMPLEMENT_APP_NO_MAIN(AtlasDLLApp)
