// ScEd.cpp : Defines the class behaviors for the application.
//

#include "precompiled.h"
#include "stdafx.h"
#include "ScEd.h"

#include "MainFrm.h"
#include "ScEdDoc.h"
#include "ScEdView.h"
#include "WebLinkButton.h"
#include "UIGlobals.h"

#include "lib.h"
#ifdef _M_IX86
#include "sysdep/ia32.h"
#endif
#include "detect.h"

/////////////////////////////////////////////////////////////////////////////
// CScEdApp

BEGIN_MESSAGE_MAP(CScEdApp, CWinApp)
	//{{AFX_MSG_MAP(CScEdApp)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
	// Standard file based document commands
	ON_COMMAND(ID_FILE_NEW, CWinApp::OnFileNew)
	ON_COMMAND(ID_FILE_OPEN, CWinApp::OnFileOpen)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CScEdApp construction

CScEdApp::CScEdApp()
{
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CScEdApp object

CScEdApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CScEdApp initialization

BOOL CScEdApp::InitInstance()
{
	extern void sced_init();
	sced_init();

	AfxEnableControlContainer();

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	//  of your final executable, you should remove from the following
	//  the specific initialization routines you do not need.

#ifdef _AFXDLL
	Enable3dControls();			// Call this when using MFC in a shared DLL
#else
	Enable3dControlsStatic();	// Call this when linking to MFC statically
#endif

	// Change the registry key under which our settings are stored.
	// TODO: You should modify this string to be something appropriate
	// such as the name of your company or organization.
	SetRegistryKey(_T("Local AppWizard-Generated Applications"));

	LoadStdProfileSettings();  // Load standard INI file options (including MRU)

	// Register the application's document templates.  Document templates
	//  serve as the connection between documents, frame windows and views.

	CSingleDocTemplate* pDocTemplate;
	pDocTemplate = new CSingleDocTemplate(
		IDR_MAINFRAME,
		RUNTIME_CLASS(CScEdDoc),
		RUNTIME_CLASS(CMainFrame),       // main SDI frame window
		RUNTIME_CLASS(CScEdView));
	AddDocTemplate(pDocTemplate);

	// Parse command line for standard shell commands, DDE, file open
	CCommandLineInfo cmdInfo;
	ParseCommandLine(cmdInfo);

	// Dispatch commands specified on the command line
	if (!ProcessShellCommand(cmdInfo))
		return FALSE;

	((CMainFrame*)m_pMainWnd)->SetTitle();

	// The one and only window has been initialized, so show and update it.
	m_pMainWnd->SetWindowPos(&m_pMainWnd->wndTop, 0, 0, 800, 600, SWP_SHOWWINDOW);
	m_pMainWnd->UpdateWindow();

	return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };
	CWebLinkButton	m_WFGLinkButton;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAboutDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	//{{AFX_MSG(CAboutDlg)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
	//{{AFX_DATA_INIT(CAboutDlg)
	//}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	DDX_Control(pDX, IDC_BUTTON_LAUNCHWFG, m_WFGLinkButton);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

// App command to run the dialog
void CScEdApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}

/////////////////////////////////////////////////////////////////////////////
// CScEdApp message handlers

BOOL CAboutDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	char buf[64];
	GetVersionString(buf);

	CWnd* wnd=GetDlgItem(IDC_STATIC_VERSION);
	wnd->SetWindowText(buf);

	m_WFGLinkButton.m_Address="www.wildfiregames.com";

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

int CScEdApp::Run()
{
	MSG msg;
	// acquire and dispatch messages until a WM_QUIT message is received
	while (1) {
		// process windows messages
		while (::PeekMessage(&msg, NULL, NULL, NULL, PM_NOREMOVE)) {
			// pump message, but quit on WM_QUIT
			if (!PumpMessage())
				return ExitInstance();
		}
		// do idle time processing
		CMainFrame* mainfrm=(CMainFrame*) AfxGetMainWnd();
		if (mainfrm) {

			// longer delay when visible but not active
			if (!mainfrm->IsTopParentActive())
				Sleep(450);

			if (!mainfrm->IsIconic()) {
				CScEdView* view=(CScEdView*) mainfrm->GetActiveView();
				if (view) {
					view->IdleTimeProcess();
				}
			}
			Sleep(50);
		}
	}

	// shouldn't get here
	return 0;
}
