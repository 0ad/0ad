/*
	MAIN.CPP
	
	Source
	
	Nemesis Multimedia Framework                   Copyright (c) Gustav Larsson
_______________________________________________________________________________
******************************************************************************/

//-----------------------------------------------------------------------------
//    IMPLEMENTATION HEADERS
//=============================================================================
//#pragma comment(lib,"../debug/nemesis.lib")
///#include "nemesis.h"
#include "GUI.h"
///using namespace NEM_STL;

//-----------------------------------------------------------------------------
//    IMPLEMENTATION PRIVATE DEFINITIONS / ENUMERATIONS / SIMPLE TYPEDEFS
//=============================================================================
//-----------------------------------------------------------------------------
//    IMPLEMENTATION PRIVATE CLASS PROTOTYPES / EXTERNAL CLASS REFERENCES
//=============================================================================
//-----------------------------------------------------------------------------
//    IMPLEMENTATION PRIVATE STRUCTURES / UTILITY CLASSES
//=============================================================================
//-----------------------------------------------------------------------------
//    IMPLEMENTATION PRIVATE DATA
//=============================================================================
//-----------------------------------------------------------------------------
//    INTERFACE DATA (GLOBALS)
//=============================================================================
///nemInput input;
///nemFontNTF font;
///nemConsoleGUIdefault consoleGUI;
//-----------------------------------------------------------------------------
//    IMPLEMENTATION PRIVATE FUNCTION PROTOTYPES
//=============================================================================
//-----------------------------------------------------------------------------
//    IMPLEMENTATION PRIVATE FUNCTIONS
//=============================================================================
/*/*
void cmd_show_console(nemConsole& c, const string& strArguments)
{
	bool bArg, bResult;	
	bResult = string2bool(strArguments, bArg);

	// Reset console string
	g_strConsole = "";

	if (strArguments == "")
	{
		// toggle
		c.pGUI->active = !c.pGUI->active;
	}
	else
	if (!bResult)
	{
		c.submit("echo Error! Invalid paramter");
	}
	else c.pGUI->active = bArg;
}
*/
CGUI gui;
CButton *button2;

//-----------------------------------------------------------------------------
//    INTERFACE FUNCTIONS
//=============================================================================

// nemInit
// ------------------------------------------------------------------| Function
// Initalization goes here
bool nemInit()
{
/*/*
	g_console.inputVariable("sys_windowName", nemConsoleVariable::createString("Test This") );

	// Init console gui
	consoleGUI.init(&font);

	g_console.inputCommand("show_console", nemConsoleCommand(&cmd_show_console));

	// Setup console gui
	g_console.initUI(&consoleGUI, &input, 100);

	g_console.submit("exec config.cfg");

	g_console.pGUI->active = true;

	// Init window, or quit
	if (!g_window.init())
		return 0;

	input.init(NEM_KEYBOARD | NEM_MOUSE);
*/

	gui.Initialize(/*/*&input*/);


	gui.LoadXMLFile("hello.xml");
	gui.LoadXMLFile("sprite1.xml");

	return true;
}

// nemInitGL
// ------------------------------------------------------------------| Function
// Initalization of GL goes here
//  will be called when switching to fullscreen and similiar
bool nemInitGL()
{
	glShadeModel(GL_SMOOTH);
	glClearColor(1.0f, 1.0f, 1.5f, 0.5f);
	glClearDepth(1.0f);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

///	font.init("Small.ntf");

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();

///	font.print(g_screenWidth/2, g_screenHeight/2, CENTER, "<black>Loading...");

///	g_window.swapBuffers();

	return true;
}

// nemMain
// ------------------------------------------------------------------| Function
// will continuously be called
bool nemMain()
{
/*/*
	input.update();
	g_console.update();
*/
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	glLoadIdentity();

	gui.Process();
	//if (res2 != PS_OK)
	//	g_console.submit("echo %s", res2);

///	if (input.kbPress(NEMK_G))
	{
		//button2 = gui.	

/*		bool hidden;
		GUI<bool>::GetSetting(gui, "Button2", "hidden", hidden);

		hidden = !hidden;

		GUI<bool>::SetSetting(gui, "Button2", "hidden", hidden);
	
		g_console.submit("echo G %s", (hidden?"true":"false"));
*/	}

//	nemPush2D();
		gui.Draw();
/*
		glDisable(GL_DEPTH_TEST);

		glBegin(GL_LINE_LOOP);
			glColor3f(1,0,0);
			glVertex2i(input.mPosX, input.mPosY);
			glVertex2i(input.mPosX, input.mPosY-18);
			glVertex2i(input.mPosX+10, input.mPosY-12);
		glEnd();

		glEnable(GL_DEPTH_TEST);

	nemPop2D();
*/
	//g_console.submit("echo --");

///	g_console.draw();

	// finish up drawing
///	g_window.swapBuffers();

	return true;
}

// nemShutdown
// ------------------------------------------------------------------| Function
// All shutdown goes here
bool nemShutdown()
{
/*/*	input.shutdown();
	font.shutdown();
	consoleGUI.shutdown();
*/
	gui.Destroy();

	return true;
}

//-----------------------------------------------------------------------------
//    INTERFACE CLASS BODIES
//=============================================================================

/* End of MAIN.CPP
******************************************************************************/
