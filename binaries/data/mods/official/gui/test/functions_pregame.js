// Main Pregame JS Script file
// Contains functions and code for Main Menu.

// ====================================================================

function openMainMenuSubWindow (windowName)
{
	// Helper function that enables the dark background mask, then reveals a given subwindow object.

	guiUnHide	("pregame_subwindow_bkg");
	guiUnHide	(windowName);
}

// ====================================================================

function closeMainMenuSubWindow (windowName)
{
	// Helper function that disables the dark background mask, then hides a given subwindow object.

	guiHide		("pregame_subwindow_bkg");
	guiHide		(windowName);
}

// ====================================================================

function initPreGame()
{
}

// ====================================================================

