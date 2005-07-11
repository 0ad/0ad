// Main Pregame JS Script file
// Contains functions and code for Main Menu.

// ====================================================================

function OpenMainMenuSubWindow(WindowName)
{
	// Helper function that enables the dark background mask, then reveals a given subwindow object.

	GUIObjectUnhide	("pregame_subwindow_bkg");
	GUIObjectUnhide	(WindowName);
}

// ====================================================================

function CloseMainMenuSubWindow(WindowName)
{
	// Helper function that disables the dark background mask, then hides a given subwindow object.

	GUIObjectHide	("pregame_subwindow_bkg");
	GUIObjectHide	(WindowName);
}

// ====================================================================

function SwitchMainMenuSubWindow(CloseWindowName, OpenWindowName)
{
	// Helper function that closes a given window (usually the current parent) and opens another one.

	GUIObjectHide	(CloseWindowName);
	GUIObjectUnhide	(OpenWindowName);
}

// ====================================================================

function initPreGame()
{
}

// ====================================================================

