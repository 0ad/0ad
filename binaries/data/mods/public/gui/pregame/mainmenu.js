var userReportEnabledText; // contains the original version with "$status" placeholder

function init()
{
	global.curr_music = newRandomSound("music", "menu");
	if (global.curr_music)
		global.curr_music.loop();

	userReportEnabledText = getGUIObjectByName("userReportEnabledText").caption;
}

function submitUserReportMessage()
{
	var input = getGUIObjectByName("userReportMessageInput");
	var msg = input.caption;
	if (msg.length)
		Engine.SubmitUserReport("message", 1, msg);
	input.caption = "";
}

function formatUserReportStatus(status)
{
	var d = status.split(/:/, 3);

	if (d[0] == "disabled")
		return "disabled";

	if (d[0] == "connecting")
		return "connecting to server";

	if (d[0] == "sending")
	{
		var done = d[1];
		return "uploading (" + Math.floor(100*done) + "%)";
	}

	if (d[0] == "completed")
	{
		var httpCode = d[1];
		if (httpCode == 200)
			return "upload succeeded";
		else
			return "upload failed (" + httpCode + ")";
	}

	if (d[0] == "failed")
	{
		var errCode = d[1];
		var errMessage = d[2];
		return "upload failed (" + errMessage + ")";
	}

	return "unknown";
}

function onTick()
{
	if (Engine.IsUserReportEnabled())
	{
		getGUIObjectByName("userReportDisabled").hidden = true;
		getGUIObjectByName("userReportEnabled").hidden = false;

		getGUIObjectByName("userReportEnabledText").caption =
			userReportEnabledText.replace(/\$status/,
				formatUserReportStatus(Engine.GetUserReportStatus()));
	}
	else
	{
		getGUIObjectByName("userReportDisabled").hidden = false;
		getGUIObjectByName("userReportEnabled").hidden = true;
	}
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
