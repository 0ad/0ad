var userReportEnabledText; // contains the original version with "$status" placeholder

function init()
{
	global.curr_music = newRandomSound("music", "menu");
	if (global.curr_music)
		global.curr_music.loop();

	userReportEnabledText = getGUIObjectByName("userReportEnabledText").caption;
}

var t0 = new Date;
function scrollBackgrounds()
{
	var layer1 = getGUIObjectByName("backgroundLayer1");
	var layer2 = getGUIObjectByName("backgroundLayer2");
	var layer3 = getGUIObjectByName("backgroundLayer3");

	var screen = layer1.parent.getComputedSize();
	var h = screen.bottom - screen.top; // height of screen
	var w = h*16/9; // width of background image

	// Offset the layers by oscillating amounts
	var t = (t0 - new Date) / 1000;
	var speed = 1/10;
	var off1 = 0.10 * w * (1+Math.cos(t*speed));
	var off2 = 0.18 * w * (1+Math.cos(t*speed)) - h*6/9;
	var off3 = 0.20 * w * (1+Math.cos(t*speed));

	var left = screen.right - w * (1 + Math.ceil(screen.right / w));
	layer1.size = new GUISize(left + off1, screen.top, screen.right + off1, screen.bottom);

	layer2.size = new GUISize(screen.right - h + off2, screen.top, screen.right + off2, screen.bottom);
	layer3.size = new GUISize(screen.right - h + off3, screen.top, screen.right + off3, screen.bottom);
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
	scrollBackgrounds();

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
