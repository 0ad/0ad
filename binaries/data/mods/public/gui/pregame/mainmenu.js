function init()
{
	global.curr_music = newRandomSound("music", "menu");
	if (global.curr_music)
		global.curr_music.loop();
}

// Helper function that enables the dark background mask, then reveals a given subwindow object.
function openMainMenuSubWindow (windowName)
{
	guiUnHide("pgSubWindow");
	guiUnHide(windowName);
}

// Helper function that disables the dark background mask, then hides a given subwindow object.
function closeMainMenuSubWindow (windowName)
{
	guiHide("pgSubWindow");
	guiHide(windowName);
}

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
