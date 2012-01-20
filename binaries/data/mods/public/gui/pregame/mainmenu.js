var userReportEnabledText; // contains the original version with "$status" placeholder
var currentSubmenuType; // contains submenu type
const MARGIN = 4; // menu border size
const background = "hellenes1"; // Background type. Currently: 'hellenes1', 'persians1'.

function init()
{
	initMusic();

	// Play main menu music
	global.music.setState(global.music.states.MENU);

	userReportEnabledText = getGUIObjectByName("userReportEnabledText").caption;

	// initialize currentSubmenuType with placeholder to avoid null when switching
	currentSubmenuType = "submenuSinglePlayer";
}

var t0 = new Date;
function scrollBackgrounds(background)
{
	if (background == "hellenes1")
	{
		var layer1 = getGUIObjectByName("backgroundHele1-1");
		var layer2 = getGUIObjectByName("backgroundHele1-2");
		var layer3 = getGUIObjectByName("backgroundHele1-3");
		
		layer1.hidden = false;
		layer2.hidden = false;
		layer3.hidden = false;
	 
		var screen = layer1.parent.getComputedSize();
		var h = screen.bottom - screen.top; // height of screen
		var w = h*16/9; // width of background image
	
		// Offset the layers by oscillating amounts
		var t = (t0 - new Date) / 700;
		var speed = 1/20;
		var off1 = 0.02 * w * (1+Math.cos(t*speed));
		var off2 = 0.12 * w * (1+Math.cos(t*speed)) - h*6/9;
		var off3 = 0.16 * w * (1+Math.cos(t*speed));
	
		var left = screen.right - w * (1 + Math.ceil(screen.right / w));
		layer1.size = new GUISize(left + off1, screen.top, screen.right + off1, screen.bottom);
		layer2.size = new GUISize(screen.right/2 - h + off2, screen.top, screen.right/2 + h + off2, screen.bottom);
		layer3.size = new GUISize(screen.right - h + off3, screen.top, screen.right + off3, screen.bottom);
	}
	
	if (background == "persians1")
	{
		var layer1 = getGUIObjectByName("backgroundPers1-1");
		var layer2 = getGUIObjectByName("backgroundPers1-2");
		var layer3 = getGUIObjectByName("backgroundPers1-3");
		var layer4 = getGUIObjectByName("backgroundPers1-4");
		
		layer1.hidden = false;
		layer2.hidden = false;
		layer3.hidden = false;
		layer4.hidden = false;
		
		var screen = layer1.parent.getComputedSize();
		var h = screen.bottom - screen.top; // height of screen
		var screenWidth = screen.right - screen.left;
		var w = h*16/9;
		
		var t = (t0 - new Date) / 1000;
		var speed = 1/20;
		var off1 = 0.01 * w * (Math.cos(t*speed));
		var off2 = 0.03 * w * (Math.cos(t*speed));
		var off3 =  0.07 * w * (1+Math.cos(t*speed)) + 0.5 * screenWidth - h*1.1;
		var off4 =  0.16 * w * (1+Math.cos(t*speed)) - h*6/9;
		
		var left = screen.right - w * (1 + Math.ceil(screen.right / w)) - 0.5 * screenWidth + h;
		layer1.size = new GUISize(left + off1, screen.top, screen.right + off1 + h, screen.bottom);
		layer2.size = new GUISize(left + off2, screen.top, screen.right + off2 + h, screen.bottom);
		layer3.size = new GUISize(screen.left + off3, screen.top, screen.left + 2 * h + off3, screen.bottom);
		layer4.size = new GUISize(screen.left + off4, screen.top, screen.left + 2 * h + off4, screen.bottom);
	}
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

var lastTickTime = new Date;
function onTick()
{
	var now = new Date;
	var tickLength = new Date - lastTickTime;
	lastTickTime = now;

	// Animate backgrounds
	scrollBackgrounds(background);

	// Animate submenu
	updateMenuPosition(tickLength);

	// Update music state
	global.music.updateTimer();

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


/*
 * MENU FUNCTIONS
 */

// Temporarily adding this here
//const BUTTON_SOUND = "audio/interface/ui/ui_button_longclick.ogg";
//function playButtonSound()
//{
//    var buttonSound = new Sound(BUTTON_SOUND);
//    buttonSound.play();
//}

// Slide menu
function updateMenuPosition(dt)
{
	var submenu = getGUIObjectByName("submenu");

	if (submenu.hidden == false)
	{
		// Number of pixels per millisecond to move
		const SPEED = 1.2;

		var maxOffset = getGUIObjectByName("mainMenu").size.right - submenu.size.left;
		if (maxOffset > 0)
		{
			var offset = Math.min(SPEED * dt, maxOffset);
			var size = submenu.size;
			size.left += offset;
			size.right += offset;
			submenu.size = size;
		}
	}
}

// Opens the menu by revealing the screen which contains the menu
function openMenu(newSubmenu, position, buttonHeight, numButtons)
{
	// switch to new submenu type
	currentSubmenuType = newSubmenu;
	getGUIObjectByName(currentSubmenuType).hidden = false;

	// set position of new submenu
	var submenu = getGUIObjectByName("submenu");
	var top = position - MARGIN;
	var bottom = position + ((buttonHeight + MARGIN) * numButtons);
	submenu.size = submenu.size.left + " " + top + " " + submenu.size.right + " " + bottom;

	// Blend in right border of main menu into the left border of the submenu
	blendSubmenuIntoMain(top, bottom);

	// Reveal submenu
	getGUIObjectByName("submenu").hidden = false;
}

// Closes the menu and resets position
function closeMenu()
{
//	playButtonSound();

	// remove old submenu type
	getGUIObjectByName(currentSubmenuType).hidden = true;

	// hide submenu and reset position
	var submenu = getGUIObjectByName("submenu");
	submenu.hidden = true;
	submenu.size = getGUIObjectByName("mainMenu").size;

	// reset main menu panel right border
	getGUIObjectByName("MainMenuPanelRightBorderTop").size = "100%-2 0 100% 100%";
}

// Sizes right border on main menu panel to match the submenu
function blendSubmenuIntoMain(topPosition, bottomPosition)
{
	var topSprite = getGUIObjectByName("MainMenuPanelRightBorderTop");
	topSprite.size = "100%-2 0 100% " + (topPosition + MARGIN);

	var bottomSprite = getGUIObjectByName("MainMenuPanelRightBorderBottom");
	bottomSprite.size = "100%-2 " + (bottomPosition) + " 100% 100%";
}

// Reveals submenu
//function openMainMenuSubWindow (windowName)
//{
//	guiUnHide("pgSubWindow");
//	guiUnHide(windowName);
//}

// Hides submenu
//function closeMainMenuSubWindow (windowName)
//{
//	guiHide("pgSubWindow");
//	guiHide(windowName);
//}



/*
 * FUNCTIONS BELOW DO NOT WORK YET
 */

//// Switch to a given options tab window.
//function openOptionsTab(tabName)
//{
//	// Hide the other tabs.
//	for (i = 1; i <= 3; i++)
//	{
//		switch (i)
//		{
//			case 1:
//				var tmpName = "pgOptionsAudio";
//			break;
//			case 2:
//				var tmpName = "pgOptionsVideo";
//			break;
//			case 3:
//				var tmpName = "pgOptionsGame";
//			break;
//			default:
//			break;
//		}
//
//		if (tmpName != tabName)
//		{
//			getGUIObjectByName (tmpName + "Window").hidden = true;
//			getGUIObjectByName (tmpName + "Button").enabled = true;
//		}
//	}
//
//	// Make given tab visible.
//	getGUIObjectByName (tabName + "Window").hidden = false;
//	getGUIObjectByName (tabName + "Button").enabled = false;
//}
//
//// Move the credits up the screen.
//function updateCredits()
//{
//	// If there are still credit lines to remove, remove them.
//	if (getNumItems("pgCredits") > 0)
//		removeItem ("pgCredits", 0);
//	else
//	{
//		// When we've run out of credit,
//
//		// Stop the increment timer if it's still active.
//		cancelInterval();
//
//		// Close the credits screen and return.
//		closeMainMenuSubWindow ("pgCredits");
//		guiUnHide ("pg");
//	}
//}
