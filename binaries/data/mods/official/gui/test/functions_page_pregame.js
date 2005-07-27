/*
	DESCRIPTION	: Main Pregame JS Script file.
	NOTES		: Contains functions and code for Main Menu.
*/

// ====================================================================

function openMainMenuSubWindow (windowName)
{
	// Helper function that enables the dark background mask, then reveals a given subwindow object.

	guiUnHide	("pgSubWindow");
	guiUnHide	(windowName);
}

// ====================================================================

function closeMainMenuSubWindow (windowName)
{
	// Helper function that disables the dark background mask, then hides a given subwindow object.

	guiHide		("pgSubWindow");
	guiHide		(windowName);
}

// ====================================================================

function updateCredits()
{
	// If there are still credit lines to remove, remove them.
	if (getNumItems("pgCredits") > 0)
		removeItem ("pgCredits", 0);
	else
	{
		// When we've run out of credit,

		// Stop the increment timer if it's still active.
		cancelInterval();

		// Close the credits screen and return.
		closeMainMenuSubWindow ("pgCredits");
		guiUnHide ("pg");
	}
}

// ====================================================================

