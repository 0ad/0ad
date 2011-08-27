function openMenu()
{
    getGUIObjectByName("menuScreen").hidden = false;
    getGUIObjectByName("menu").hidden = false;
}

function closeMenu()
{
    getGUIObjectByName("menuScreen").hidden = true;
    getGUIObjectByName("menu").hidden = true;
}

function openSettings()
{
        closeMenu();
        closeChat();
        getGUIObjectByName("settingsDialogPanel").hidden = false;
}

function closeSettings()
{
        getGUIObjectByName("settingsDialogPanel").hidden = true;
}

function openChat()
{
        closeMenu();
        closeSettings();
	getGUIObjectByName("chatInput").focus(); // Grant focus to the input area
	getGUIObjectByName("chatDialogPanel").hidden = false;
	
}

function closeChat()
{
	getGUIObjectByName("chatInput").caption = ""; // Clear chat input
	getGUIObjectByName("chatDialogPanel").hidden = true;
}

function toggleChatWindow()
{
        closeSettings();
    
	var chatWindow = getGUIObjectByName("chatDialogPanel");
	var chatInput = getGUIObjectByName("chatInput");

	if (chatWindow.hidden)
		chatInput.focus(); // Grant focus to the input area
	else
		chatInput.caption = ""; // Clear chat input

	chatWindow.hidden = !chatWindow.hidden;
}

function togglePause()
{
        closeMenu();
        closeChat();
        closeSettings();
    
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
}

function toggleDeveloperOverlay()
{
	var devCommands = getGUIObjectByName("devCommands");
	var text = devCommands.hidden? "opened." : "closed.";
	submitChatDirectly("The Developer Overlay was " + text);
	devCommands.hidden = !devCommands.hidden;
}

function escapeKeyAction()
{
        closeMenu();
        closeChat();
        closeSettings();
}