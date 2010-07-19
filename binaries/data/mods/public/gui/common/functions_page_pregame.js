/*
	DESCRIPTION	: Main Pregame JS Script file.
	NOTES		: Contains functions and code for Main Menu.
*/

// ====================================================================

// Switch from the archive builder to the main menu.
function startMainMenu()
{
	// Reveal the main menu now that the archive has been loaded.
	guiHide ("ab");
	guiUnHide ("pg");

	// Play main 0 A.D. theme when the main menu starts.
	global.curr_music = newRandomSound("music", "menu");
	if (global.curr_music)
		global.curr_music.loop();
	
	// Set starting volume (I'm using a value of zero here for no sound; feel free to comment out these two lines to use defaults).
//	global.curr_music.setGain (0.0);
//	g_ConfigDB.system["sound.mastergain"] = 0.0;	
}

// ====================================================================

// Helper function that enables the dark background mask, then reveals a given subwindow object.
function openMainMenuSubWindow (windowName)
{
	guiUnHide	("pgSubWindow");
	guiUnHide	(windowName);
}

// ====================================================================

// Helper function that disables the dark background mask, then hides a given subwindow object.
function closeMainMenuSubWindow (windowName)
{
	guiHide		("pgSubWindow");
	guiHide		(windowName);
}

// ====================================================================

// Switch to a given options tab window.
function openOptionsTab(tabName)
{
	// Hide the other tabs.
	for (i = 1; i <= 3; i++)
	{
		switch (i)
		{
			case 1:
				var tmpName = "pgOptionsAudio";
			break;
			case 2:
				var tmpName = "pgOptionsVideo";
			break;
			case 3:
				var tmpName = "pgOptionsGame";
			break;
			default:
			break;
		}

		if (tmpName != tabName)
		{
			getGUIObjectByName (tmpName + "Window").hidden = true;
			getGUIObjectByName (tmpName + "Button").enabled = true;
		}
	}

	// Make given tab visible.
	getGUIObjectByName (tabName + "Window").hidden = false;
	getGUIObjectByName (tabName + "Button").enabled = false;
}

// ====================================================================

// Move the credits up the screen.
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

