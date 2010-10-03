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

function escapeKeyAction() // runs multiple times, so always closes all for now...
{
	if (!menu.hidden)
		menu.hidden = true;
	else if (!chatWindow.hidden)
		chatWindow.hidden = true;
	else if (!settingsWindow.hidden)
		settingsWindow.hidden = true;
}
