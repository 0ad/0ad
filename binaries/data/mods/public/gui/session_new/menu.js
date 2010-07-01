function toggleDeveloperOverlay()
{
	if (getGUIObjectByName("devCommands").hidden)
		getGUIObjectByName("devCommands").hidden = false; // show overlay
	else
		getGUIObjectByName("devCommands").hidden = true; // hide overlay
}

function toggleSettingsWindow()
{
	if (getGUIObjectByName("settingsWindow").hidden)
	{
		getGUIObjectByName("settingsWindow").hidden = false; // show settings
		setPaused(true);
	}
	else
	{
		getGUIObjectByName("settingsWindow").hidden = true; // hide settings
		setPaused(false);
	}

	getGUIObjectByName("menu").hidden = true; // Hide menu
}

function togglePause()
{
	if (getGUIObjectByName("pauseOverlay").hidden)
	{
		getGUIObjectByName("pauseOverlay").hidden = false; // pause game
		setPaused(true);
	}
	else
	{
		getGUIObjectByName("pauseOverlay").hidden = true; // unpause game
		setPaused(false);
	}

	getGUIObjectByName("menu").hidden = true; // Hide menu
}

function toggleMenu()
{
	if (getGUIObjectByName("menu").hidden)
		getGUIObjectByName("menu").hidden = false; // View menu
	else
		getGUIObjectByName("menu").hidden = true; // Hide menu
}
