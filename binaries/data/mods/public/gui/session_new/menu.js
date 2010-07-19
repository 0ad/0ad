// Group Selection by Rank
var g_GroupSelectionByRank = true; // referenced in EntityGroups.createGroups(ents) AND in setupUnitPanel(...)
function groupSelectionByRank(booleanValue)
{
	g_GroupSelectionByRank = booleanValue;
	g_Selection.groups.createGroups(g_Selection.toList());
}

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
		getGUIObjectByName("settingsWindow").hidden = false; // show settings
	else
		getGUIObjectByName("settingsWindow").hidden = true; // hide settings

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