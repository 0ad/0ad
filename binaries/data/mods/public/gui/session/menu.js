function toggleDeveloperOverlay()
{
	var devCommands = getGUIObjectByName("devCommands");
	var text = devCommands.hidden? "opened." : "closed.";
	submitChatDirectly("The Developer Overlay was " + text);
	devCommands.hidden = !devCommands.hidden;
}

function openMenuDialog()
{
	var menu = getGUIObjectByName("menuDialogPanel");
	g_SessionDialog.open("Menu", null, menu, 156, 224, null);
}

function openSettingsDialog()
{
	var settings = getGUIObjectByName("settingsDialogPanel");
	g_SessionDialog.open("Settings", null, settings, 340, 252, null);
}

function openChat()
{
	getGUIObjectByName("chatInput").focus(); // Grant focus to the input area
	getGUIObjectByName("chatDialogPanel").hidden = false;
	g_SessionDialog.close();
}

function closeChat()
{
	getGUIObjectByName("chatInput").caption = ""; // Clear chat input
	getGUIObjectByName("chatDialogPanel").hidden = true;
	g_SessionDialog.close();
}

function toggleChatWindow()
{
	var chatWindow = getGUIObjectByName("chatDialogPanel");
	var chatInput = getGUIObjectByName("chatInput");

	if (chatWindow.hidden)
		chatInput.focus(); // Grant focus to the input area
	else
		chatInput.caption = ""; // Clear chat input

	chatWindow.hidden = !chatWindow.hidden;
	g_SessionDialog.close();
}

function togglePause()
{
	var pauseOverlay = getGUIObjectByName("pauseOverlay");

	if (pauseOverlay.hidden)
	{
		setPaused(true);
		getGUIObjectByName("pauseButtonText").caption = "Unpause";
	}
	else
	{
		setPaused(false);
		getGUIObjectByName("pauseButtonText").caption = "Pause";
	}

	pauseOverlay.hidden = !pauseOverlay.hidden;
	g_SessionDialog.close();
}

function openExitGameDialog()
{
	g_SessionDialog.open("Exit Game", "Do you really want to quit?", null, 320, 140, leaveGame);
}

function escapeKeyAction()
{
	var sessionDialog =  getGUIObjectByName("sessionDialog");
	
	if (!sessionDialog.hidden)
		g_SessionDialog.close();
	else
		getGUIObjectByName("chatDialogPanel").hidden = true;
}
