// Cache these objects for future access
var devCommands;
var menu;
var settingsWindow;
var chatWindow;
var chatInput;
var pauseOverlay;

function cacheMenuObjects()
{
	devCommands = getGUIObjectByName("devCommands");
	menu = getGUIObjectByName("menuPanel");
	settingsWindow = getGUIObjectByName("settingsWindow");
	chatWindow = getGUIObjectByName("chatWindow");
	chatInput = getGUIObjectByName("chatInput");
	pauseOverlay = getGUIObjectByName("pauseOverlay");
}

function toggleDeveloperOverlay()
{
	var text = devCommands.hidden? "opened." : "closed.";
	submitChatDirectly("The Developer Overlay was " + text);
	
	devCommands.hidden = !devCommands.hidden;
}

function toggleMenu()
{
	menu.hidden = !menu.hidden;
}

function toggleSettingsWindow()
{
	settingsWindow.hidden = !settingsWindow.hidden;
	menu.hidden = true;
}

function toggleChatWindow()
{
	if (chatWindow.hidden)
		chatInput.focus(); // Grant focus to the input area
	else
		chatInput.caption = ""; // Clear chat input

	chatWindow.hidden = !chatWindow.hidden;
	menu.hidden = true;
}

function togglePause()
{
	if (pauseOverlay.hidden)
		setPaused(true);
	else
		setPaused(false);

	pauseOverlay.hidden = !pauseOverlay.hidden;
	menu.hidden = true;
}

function openExitGameDialog()
{
	g_SessionDialog.open("Confirmation", "Do you really want to quit?", null, 160, 70, leaveGame);
}

function escapeKeyAction()
{
	if (!menu.hidden)
	{
		menu.hidden = true;
	}
	else if (!chatWindow.hidden)
	{
		chatWindow.hidden = true;
		chatInput.caption = "";
	}
	else if (!settingsWindow.hidden)
	{
		settingsWindow.hidden = true;
		console.write("test");
	}
}
